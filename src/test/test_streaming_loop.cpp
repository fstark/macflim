#include "../doctest.h"

#include "../bitmap.hpp"
#include "../codec_spec.hpp"
#include "../decoder.hpp"
#include "../encode_frame.hpp"
#include "../streaming/client_state_tracker.hpp"

#include <cstdint>
#include <random>
#include <vector>

namespace macflim
{

namespace
{

constexpr size_t W = 512;
constexpr size_t H = 342;
constexpr size_t BUDGET = 2000;

std::vector<codec_spec> test_codecs()
{
    return {make_codec("z32", W, H), make_codec("invert", W, H), make_codec("null", W, H)};
}

/// Simulate what the real client does: apply deltas it received, skip missed ones.
/// Returns the client's actual screen state.
bitmap simulate_client(const bitmap &initial, const std::vector<std::vector<uint8_t>> &deltas,
                       const std::vector<bool> &received)
{
    bitmap screen = initial;
    for (size_t i = 0; i < deltas.size(); ++i)
        if (received[i])
            apply_delta(screen, deltas[i]);
    return screen;
}

/// Build a history byte vector from a received[] vector for a feedback packet.
/// received[0] is the oldest frame (seq = base_seq+1), received.back() is last_displayed_seq.
/// The history bitmap encodes bits relative to last_displayed_seq: bit 0 = last_displayed_seq,
/// bit 1 = last_displayed_seq-1, etc.
std::vector<uint8_t> build_history(const std::vector<bool> &received, size_t num_bits = 128)
{
    std::vector<uint8_t> history(num_bits / 8, 0x00);
    for (size_t i = 0; i < received.size() && i < num_bits; ++i)
    {
        //  received[i] corresponds to offset (received.size()-1-i) from last_displayed_seq
        size_t bit_offset = received.size() - 1 - i;
        if (bit_offset < num_bits && received[i])
        {
            size_t byte_index = bit_offset / 8;
            uint8_t bit_mask = 1u << (bit_offset % 8);
            history[byte_index] |= bit_mask;
        }
    }
    return history;
}

/// Generate a sequence of distinct target bitmaps from a seed, simulating video frames.
std::vector<bitmap> make_target_sequence(size_t count, int base_seed)
{
    std::vector<bitmap> targets;
    targets.reserve(count);
    for (size_t i = 0; i < count; ++i)
    {
        bitmap bm(W, H);
        bm.randomize(base_seed + static_cast<int>(i));
        targets.push_back(std::move(bm));
    }
    return targets;
}

} // namespace

// ---------------------------------------------------------------------------
// Core integration: encode → track → client-replay → feedback → re-encode
// ---------------------------------------------------------------------------

TEST_CASE("streaming loop: no loss — tracker matches client perfectly")
{
    auto codecs = test_codecs();
    auto targets = make_target_sequence(10, 100);
    bitmap initial(W, H);
    initial.fill(0x00);

    //  Server-side: encoder + tracker
    bitmap encoder_fb = initial;
    client_state_tracker tracker(initial);
    std::vector<std::vector<uint8_t>> all_deltas;

    //  Client-side: independent replay
    bitmap client_fb = initial;

    for (size_t i = 0; i < targets.size(); ++i)
    {
        //  Encode against tracker's view of client screen
        encoder_fb = tracker.current_client_screen();
        auto result = encode_frame(encoder_fb, targets[i], codecs, BUDGET);
        auto delta = result.get_video_encoded_data();

        tracker.record_sent(delta);
        all_deltas.push_back(delta);

        //  Client receives and applies
        apply_delta(client_fb, delta);
    }

    //  Feedback: all displayed
    std::vector<bool> received(targets.size(), true);
    auto history = build_history(received);
    tracker.process_feedback(static_cast<uint32_t>(targets.size()), history);

    CHECK(tracker.current_client_screen() == client_fb);
    CHECK(tracker.simulated_seq() == targets.size());
    CHECK(tracker.in_flight_count() == 0);
}

TEST_CASE("streaming loop: single frame loss — tracker reconstructs damaged state")
{
    auto codecs = test_codecs();
    auto targets = make_target_sequence(8, 200);
    bitmap initial(W, H);
    initial.fill(0x00);

    bitmap encoder_fb = initial;
    client_state_tracker tracker(initial);
    std::vector<std::vector<uint8_t>> all_deltas;

    for (size_t i = 0; i < targets.size(); ++i)
    {
        encoder_fb = tracker.current_client_screen();
        auto result = encode_frame(encoder_fb, targets[i], codecs, BUDGET);
        auto delta = result.get_video_encoded_data();
        tracker.record_sent(delta);
        all_deltas.push_back(delta);
    }

    //  Client missed frame 4 (index 3)
    std::vector<bool> received(targets.size(), true);
    received[3] = false;

    bitmap client_fb = simulate_client(initial, all_deltas, received);

    auto history = build_history(received);
    tracker.process_feedback(static_cast<uint32_t>(targets.size()), history);

    CHECK(tracker.current_client_screen() == client_fb);
}

TEST_CASE("streaming loop: burst loss — multiple consecutive frames missed")
{
    auto codecs = test_codecs();
    auto targets = make_target_sequence(12, 300);
    bitmap initial(W, H);
    initial.fill(0x00);

    bitmap encoder_fb = initial;
    client_state_tracker tracker(initial);
    std::vector<std::vector<uint8_t>> all_deltas;

    for (size_t i = 0; i < targets.size(); ++i)
    {
        encoder_fb = tracker.current_client_screen();
        auto result = encode_frame(encoder_fb, targets[i], codecs, BUDGET);
        auto delta = result.get_video_encoded_data();
        tracker.record_sent(delta);
        all_deltas.push_back(delta);
    }

    //  Client missed frames 4,5,6 (indices 3,4,5) — burst loss
    std::vector<bool> received(targets.size(), true);
    received[3] = false;
    received[4] = false;
    received[5] = false;

    bitmap client_fb = simulate_client(initial, all_deltas, received);

    auto history = build_history(received);
    tracker.process_feedback(static_cast<uint32_t>(targets.size()), history);

    CHECK(tracker.current_client_screen() == client_fb);
}

TEST_CASE("streaming loop: incremental feedback — encode continues between feedback packets")
{
    auto codecs = test_codecs();
    auto targets = make_target_sequence(12, 400);
    bitmap initial(W, H);
    initial.fill(0x00);

    bitmap encoder_fb = initial;
    client_state_tracker tracker(initial);
    std::vector<std::vector<uint8_t>> all_deltas;
    std::vector<bool> client_received;
    bitmap client_fb = initial;

    //  Phase 1: send frames 1–6, no feedback yet
    for (size_t i = 0; i < 6; ++i)
    {
        encoder_fb = tracker.current_client_screen();
        auto result = encode_frame(encoder_fb, targets[i], codecs, BUDGET);
        auto delta = result.get_video_encoded_data();
        tracker.record_sent(delta);
        all_deltas.push_back(delta);

        //  Client receives all except frame 3 (index 2)
        bool got_it = (i != 2);
        client_received.push_back(got_it);
        if (got_it)
            apply_delta(client_fb, delta);
    }

    CHECK(tracker.in_flight_count() == 6);

    //  Feedback arrives for frames 1–6 (frame 3 missed)
    auto history1 = build_history(client_received);
    tracker.process_feedback(6, history1);

    CHECK(tracker.simulated_seq() == 6);
    CHECK(tracker.in_flight_count() == 0);
    CHECK(tracker.current_client_screen() == client_fb);

    //  Phase 2: encode and send frames 7–12, now encoding against corrected state
    for (size_t i = 6; i < 12; ++i)
    {
        encoder_fb = tracker.current_client_screen();
        auto result = encode_frame(encoder_fb, targets[i], codecs, BUDGET);
        auto delta = result.get_video_encoded_data();
        tracker.record_sent(delta);
        all_deltas.push_back(delta);

        //  Client receives all phase-2 frames
        client_received.push_back(true);
        apply_delta(client_fb, delta);
    }

    //  Feedback for frames 7–12 (all displayed)
    auto history2 = build_history(client_received);
    tracker.process_feedback(12, history2);

    CHECK(tracker.current_client_screen() == client_fb);
    CHECK(tracker.simulated_seq() == 12);
    CHECK(tracker.in_flight_count() == 0);
}

TEST_CASE("streaming loop: self-correcting property — quality improves after loss feedback")
{
    auto codecs = test_codecs();
    bitmap initial(W, H);
    initial.fill(0x00);

    //  Create a stable target — all frames want the same image
    bitmap target(W, H);
    target.randomize(500);

    bitmap encoder_fb = initial;
    client_state_tracker tracker(initial);
    std::vector<std::vector<uint8_t>> all_deltas;
    std::vector<bool> client_received;
    bitmap client_fb = initial;

    //  Phase 1: 5 frames, frame 2 missed. Server doesn't know yet.
    for (size_t i = 0; i < 5; ++i)
    {
        encoder_fb = tracker.current_client_screen();
        auto result = encode_frame(encoder_fb, target, codecs, BUDGET);
        auto delta = result.get_video_encoded_data();
        tracker.record_sent(delta);
        all_deltas.push_back(delta);

        bool got_it = (i != 1);
        client_received.push_back(got_it);
        if (got_it)
            apply_delta(client_fb, delta);
    }

    double quality_before_feedback = client_fb.proximity(target);

    //  Feedback arrives: server learns about the miss
    auto history = build_history(client_received);
    tracker.process_feedback(5, history);
    CHECK(tracker.current_client_screen() == client_fb);

    //  Phase 2: 10 more frames, all received. Server now encodes against the true damaged state.
    for (size_t i = 0; i < 10; ++i)
    {
        encoder_fb = tracker.current_client_screen();
        auto result = encode_frame(encoder_fb, target, codecs, BUDGET);
        auto delta = result.get_video_encoded_data();
        tracker.record_sent(delta);

        client_received.push_back(true);
        apply_delta(client_fb, delta);

        //  Immediate feedback for simplicity
        auto h = build_history(client_received);
        tracker.process_feedback(static_cast<uint32_t>(client_received.size()), h);
    }

    double quality_after_recovery = client_fb.proximity(target);

    //  Quality should have improved: the self-correcting deltas fix the damage
    CHECK(quality_after_recovery > quality_before_feedback);
    //  Should be quite close to the target after 10 corrective frames
    CHECK(quality_after_recovery > 0.8);
}

TEST_CASE("streaming loop: random loss pattern — tracker always matches client")
{
    auto codecs = test_codecs();
    auto targets = make_target_sequence(30, 600);
    bitmap initial(W, H);
    initial.fill(0x00);

    bitmap encoder_fb = initial;
    client_state_tracker tracker(initial);
    std::vector<std::vector<uint8_t>> all_deltas;
    std::vector<bool> client_received;
    bitmap client_fb = initial;

    //  ~20% random packet loss
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist(0, 4);

    for (size_t i = 0; i < targets.size(); ++i)
    {
        encoder_fb = tracker.current_client_screen();
        auto result = encode_frame(encoder_fb, targets[i], codecs, BUDGET);
        auto delta = result.get_video_encoded_data();
        tracker.record_sent(delta);
        all_deltas.push_back(delta);

        bool got_it = (dist(rng) != 0); // ~20% loss
        client_received.push_back(got_it);
        if (got_it)
            apply_delta(client_fb, delta);

        //  Feedback every 5 frames
        if ((i + 1) % 5 == 0)
        {
            auto history = build_history(client_received);
            tracker.process_feedback(static_cast<uint32_t>(i + 1), history);
            CHECK(tracker.current_client_screen() == client_fb);
        }
    }

    //  Final feedback
    auto history = build_history(client_received);
    tracker.process_feedback(static_cast<uint32_t>(targets.size()), history);
    CHECK(tracker.current_client_screen() == client_fb);
}

} // namespace macflim
