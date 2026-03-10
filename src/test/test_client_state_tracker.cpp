#include "../doctest.h"

#include "../bitmap.hpp"
#include "../codec_spec.hpp"
#include "../decoder.hpp"
#include "../encode_frame.hpp"
#include "../streaming/client_state_tracker.hpp"

#include <cstdint>
#include <vector>

namespace macflim
{

namespace
{

/// Helper: build a history byte vector where all frames are displayed (all bits = 1).
std::vector<uint8_t> all_displayed(size_t num_bits = 128)
{
    return std::vector<uint8_t>(num_bits / 8, 0xFF);
}

/// Helper: build a history byte vector with specific frames missed.
/// missed_offsets are bit offsets from last_displayed_seq (0 = last_displayed_seq itself).
std::vector<uint8_t> with_misses(const std::vector<uint32_t> &missed_offsets, size_t num_bits = 128)
{
    auto history = all_displayed(num_bits);
    for (auto offset : missed_offsets)
    {
        size_t byte_index = offset / 8;
        uint8_t bit_mask = 1u << (offset % 8);
        if (byte_index < history.size())
            history[byte_index] &= ~bit_mask;
    }
    return history;
}

/// Helper: encode a frame and return the delta bytes (with 4-byte codec header).
std::vector<uint8_t> make_delta(bitmap &current, const bitmap &target, const std::vector<codec_spec> &codecs,
                                size_t budget)
{
    auto result = encode_frame(current, target, codecs, budget);
    return result.get_video_encoded_data();
}

/// Helper: create a bitmap with a specific fill byte.
bitmap make_filled(size_t W, size_t H, uint8_t value)
{
    bitmap bm(W, H);
    bm.fill(value);
    return bm;
}

/// Standard codecs for testing (512x342).
std::vector<codec_spec> test_codecs()
{
    constexpr size_t W = 512;
    constexpr size_t H = 342;
    return {make_codec("z32", W, H), make_codec("invert", W, H), make_codec("null", W, H)};
}

} // namespace

TEST_CASE("client_state_tracker: fresh tracker returns initial screen")
{
    bitmap blank(512, 342);
    client_state_tracker tracker(blank);

    CHECK(tracker.current_client_screen() == blank);
    CHECK(tracker.simulated_seq() == 0);
    CHECK(tracker.in_flight_count() == 0);
}

TEST_CASE("client_state_tracker: record_sent makes frame visible in current_client_screen")
{
    auto codecs = test_codecs();
    bitmap current = make_filled(512, 342, 0x00);
    bitmap target = make_filled(512, 342, 0xFF);

    //  Encode a frame
    bitmap encoder_fb = current;
    auto delta = make_delta(encoder_fb, target, codecs, 2000);

    //  Tracker starts from the same initial screen
    client_state_tracker tracker(current);
    tracker.record_sent(1, delta);

    //  current_client_screen should reflect the delta optimistically
    CHECK(tracker.current_client_screen() == encoder_fb);
    CHECK(tracker.in_flight_count() == 1);
}

TEST_CASE("client_state_tracker: all frames acked advances simulated state")
{
    auto codecs = test_codecs();
    bitmap current = make_filled(512, 342, 0x00);
    bitmap target = make_filled(512, 342, 0xFF);

    bitmap encoder_fb = current;
    auto delta1 = make_delta(encoder_fb, target, codecs, 2000);
    auto delta2 = make_delta(encoder_fb, target, codecs, 2000);

    client_state_tracker tracker(current);
    tracker.record_sent(1, delta1);
    tracker.record_sent(2, delta2);

    //  Feedback: both displayed
    tracker.process_feedback(2, all_displayed());

    CHECK(tracker.simulated_seq() == 2);
    CHECK(tracker.in_flight_count() == 0);

    //  After feedback, current_client_screen matches the fully-applied state
    bitmap client_fb = current;
    apply_delta(client_fb, delta1);
    apply_delta(client_fb, delta2);
    CHECK(tracker.current_client_screen() == client_fb);
}

TEST_CASE("client_state_tracker: missed frame produces deterministic wrong state")
{
    auto codecs = test_codecs();
    bitmap initial = make_filled(512, 342, 0x00);
    bitmap target1 = make_filled(512, 342, 0xAA);
    bitmap target2 = make_filled(512, 342, 0x55);
    bitmap target3 = make_filled(512, 342, 0xFF);

    //  Encode three frames sequentially
    bitmap encoder_fb = initial;
    auto delta1 = make_delta(encoder_fb, target1, codecs, 2000);
    auto delta2 = make_delta(encoder_fb, target2, codecs, 2000);
    auto delta3 = make_delta(encoder_fb, target3, codecs, 2000);

    client_state_tracker tracker(initial);
    tracker.record_sent(1, delta1);
    tracker.record_sent(2, delta2);
    tracker.record_sent(3, delta3);

    //  Feedback: frame 2 missed (offset 1 from last_displayed_seq=3)
    auto history = with_misses({1}); // bit 1 = seq 2 = missed
    tracker.process_feedback(3, history);

    //  Simulate what the client actually did: apply F1, skip F2, apply F3
    bitmap client_fb = initial;
    apply_delta(client_fb, delta1);
    // skip delta2
    apply_delta(client_fb, delta3);

    CHECK(tracker.current_client_screen() == client_fb);
    CHECK(tracker.simulated_seq() == 3);
    CHECK(tracker.in_flight_count() == 0);
}

TEST_CASE("client_state_tracker: section 6 worked example — F5-F9 in flight, F7 missed")
{
    auto codecs = test_codecs();
    constexpr size_t W = 512;
    constexpr size_t H = 342;

    //  Build an initial state (simulating "after frame 4")
    bitmap after_f4 = make_filled(W, H, 0x33);

    //  Create distinct targets for frames 5-9
    bitmap targets[] = {
        make_filled(W, H, 0x11), make_filled(W, H, 0x22), make_filled(W, H, 0x44),
        make_filled(W, H, 0x88), make_filled(W, H, 0xCC),
    };

    //  Encode frames 5-9 sequentially (server side)
    bitmap encoder_fb = after_f4;
    std::vector<std::vector<uint8_t>> deltas;
    for (size_t i = 0; i < 5; i++)
        deltas.push_back(make_delta(encoder_fb, targets[i], codecs, 2000));

    //  Set up tracker at "after frame 4" with seq=4
    client_state_tracker tracker(after_f4);
    for (size_t i = 0; i < 5; i++)
        tracker.record_sent(static_cast<uint32_t>(5 + i), deltas[i]);

    CHECK(tracker.in_flight_count() == 5);

    //  Feedback: last_displayed=9, F7 missed (offset = 9-7 = 2)
    auto history = with_misses({2}); // bit 2 = seq 7 = missed
    tracker.process_feedback(9, history);

    //  Simulate the client: apply F5, F6, skip F7, apply F8, F9
    bitmap client_fb = after_f4;
    apply_delta(client_fb, deltas[0]); // F5
    apply_delta(client_fb, deltas[1]); // F6
    // skip F7 (deltas[2])
    apply_delta(client_fb, deltas[3]); // F8 — wrong base, but deterministic
    apply_delta(client_fb, deltas[4]); // F9

    CHECK(tracker.current_client_screen() == client_fb);
    CHECK(tracker.simulated_seq() == 9);
    CHECK(tracker.in_flight_count() == 0);
}

TEST_CASE("client_state_tracker: stale feedback is ignored")
{
    bitmap initial(512, 342);
    client_state_tracker tracker(initial);

    //  Process feedback to advance to seq 5
    tracker.process_feedback(5, all_displayed());
    CHECK(tracker.simulated_seq() == 5);

    //  Stale feedback (seq 3) should be ignored
    tracker.process_feedback(3, all_displayed());
    CHECK(tracker.simulated_seq() == 5);

    //  Equal seq should also be ignored
    tracker.process_feedback(5, all_displayed());
    CHECK(tracker.simulated_seq() == 5);
}

TEST_CASE("client_state_tracker: partial feedback leaves remaining frames in flight")
{
    auto codecs = test_codecs();
    bitmap initial = make_filled(512, 342, 0x00);
    bitmap target = make_filled(512, 342, 0xFF);

    //  Encode 5 frames
    bitmap encoder_fb = initial;
    std::vector<std::vector<uint8_t>> deltas;
    for (uint32_t i = 1; i <= 5; i++)
        deltas.push_back(make_delta(encoder_fb, target, codecs, 2000));

    client_state_tracker tracker(initial);
    for (uint32_t i = 1; i <= 5; i++)
        tracker.record_sent(i, deltas[i - 1]);

    //  Partial feedback: only through seq 3
    tracker.process_feedback(3, all_displayed());
    CHECK(tracker.simulated_seq() == 3);
    CHECK(tracker.in_flight_count() == 2); // F4 and F5 remain

    //  current_client_screen includes F4 and F5 optimistically
    bitmap expected = initial;
    for (const auto &d : deltas)
        apply_delta(expected, d);
    CHECK(tracker.current_client_screen() == expected);
}

TEST_CASE("client_state_tracker: progressive feedback in two steps")
{
    auto codecs = test_codecs();
    bitmap initial = make_filled(512, 342, 0x00);
    bitmap t1 = make_filled(512, 342, 0xAA);
    bitmap t2 = make_filled(512, 342, 0x55);
    bitmap t3 = make_filled(512, 342, 0xFF);

    bitmap encoder_fb = initial;
    auto d1 = make_delta(encoder_fb, t1, codecs, 2000);
    auto d2 = make_delta(encoder_fb, t2, codecs, 2000);
    auto d3 = make_delta(encoder_fb, t3, codecs, 2000);

    client_state_tracker tracker(initial);
    tracker.record_sent(1, d1);
    tracker.record_sent(2, d2);
    tracker.record_sent(3, d3);

    //  First feedback: through seq 2, all displayed
    tracker.process_feedback(2, all_displayed());
    CHECK(tracker.simulated_seq() == 2);
    CHECK(tracker.in_flight_count() == 1);

    //  Second feedback: through seq 3, all displayed
    tracker.process_feedback(3, all_displayed());
    CHECK(tracker.simulated_seq() == 3);
    CHECK(tracker.in_flight_count() == 0);

    //  Final state matches full client replay
    bitmap client_fb = initial;
    apply_delta(client_fb, d1);
    apply_delta(client_fb, d2);
    apply_delta(client_fb, d3);
    CHECK(tracker.current_client_screen() == client_fb);
}

TEST_CASE("client_state_tracker: empty in-flight with feedback beyond range")
{
    bitmap initial(512, 342);
    client_state_tracker tracker(initial);

    //  No frames recorded, but feedback says seq 10 — should just advance seq
    tracker.process_feedback(10, all_displayed());
    CHECK(tracker.simulated_seq() == 10);
    CHECK(tracker.in_flight_count() == 0);
    CHECK(tracker.current_client_screen() == initial);
}

} // namespace macflim
