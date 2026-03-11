#include "../doctest.h"

#include "../bitmap.hpp"
#include "../codec_spec.hpp"
#include "../streaming/frame_source.hpp"
#include "../streaming/protocol.hpp"
#include "../streaming/streaming_session.hpp"
#include "../streaming/transport.hpp"

#include <deque>
#include <memory>
#include <optional>
#include <vector>

using namespace macflim;

// ---------------------------------------------------------------------------
// Loopback transport — captures sent frames, allows injecting feedback
// ---------------------------------------------------------------------------

namespace
{

class loopback_transport : public transport
{
  public:
    void send_hello_ack(const hello_ack_packet & /*ack*/) override
    {
        // Not used in these tests
    }

    void send_frame(const frame_header &hdr, std::span<const uint8_t> video_data) override
    {
        sent_frames.push_back({hdr, std::vector<uint8_t>(video_data.begin(), video_data.end())});
    }

    std::optional<feedback_packet> receive_feedback() override
    {
        if (pending_feedback.empty())
            return std::nullopt;
        auto fb = pending_feedback.front();
        pending_feedback.pop_front();
        return fb;
    }

    struct sent_frame
    {
        frame_header header;
        std::vector<uint8_t> video;
    };

    std::vector<sent_frame> sent_frames;
    std::deque<feedback_packet> pending_feedback;
};

// Build a frame source from a vector of bitmaps
frame_source make_test_source(std::vector<bitmap> targets)
{
    auto shared = std::make_shared<std::vector<bitmap>>(std::move(targets));
    auto index = std::make_shared<size_t>(0);
    return [shared, index]() -> std::optional<bitmap>
    {
        if (*index >= shared->size())
            return std::nullopt;
        return (*shared)[(*index)++];
    };
}

// Build a simple set of codecs for testing (z32 only, 512x342)
std::vector<codec_spec> test_codecs(size_t W = 512, size_t H = 342)
{
    return {{0x02, 1.0, make_z32_codec(W, H)}};
}

// Create a bitmap with a known pattern
bitmap make_pattern(size_t W, size_t H, uint8_t fill)
{
    bitmap bm(W, H);
    bm.fill(fill);
    return bm;
}

// Build a feedback packet where all frames up to last_seq are marked as displayed
feedback_packet make_all_displayed(uint32_t last_seq)
{
    feedback_packet fb;
    fb.last_displayed_seq = last_seq;
    fb.history.fill(0xFF); // all bits set = all displayed
    return fb;
}

} // namespace

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

TEST_CASE("streaming_session — single frame encode and send")
{
    constexpr size_t W = 512, H = 342;
    auto targets = std::vector<bitmap>{make_pattern(W, H, 0x00)};
    auto tp = std::make_unique<loopback_transport>();
    auto *tp_ptr = tp.get();

    streaming_session session(make_test_source(std::move(targets)), test_codecs(W, H), 6000, std::move(tp), W, H);

    CHECK(session.step() == step_result::ok);
    CHECK(session.step() == step_result::finished);
    CHECK(tp_ptr->sent_frames.size() == 1);
    CHECK(tp_ptr->sent_frames[0].header.seq == 1);
    CHECK(tp_ptr->sent_frames[0].header.ticks == 1);
    CHECK(!tp_ptr->sent_frames[0].video.empty());
}

TEST_CASE("streaming_session — multiple frames")
{
    constexpr size_t W = 512, H = 342;
    std::vector<bitmap> targets;
    for (int i = 0; i < 10; ++i)
        targets.push_back(make_pattern(W, H, i % 2 == 0 ? 0x00 : 0xFF));

    auto tp = std::make_unique<loopback_transport>();
    auto *tp_ptr = tp.get();

    streaming_session session(make_test_source(std::move(targets)), test_codecs(W, H), 6000, std::move(tp), W, H);

    for (int i = 0; i < 10; ++i)
        CHECK(session.step() == step_result::ok);

    CHECK(session.step() == step_result::finished);
    CHECK(tp_ptr->sent_frames.size() == 10);

    // Sequences are monotonically increasing
    for (size_t i = 0; i < 10; ++i)
        CHECK(tp_ptr->sent_frames[i].header.seq == i + 1);
}

TEST_CASE("streaming_session — feedback updates tracker")
{
    constexpr size_t W = 512, H = 342;
    std::vector<bitmap> targets;
    for (int i = 0; i < 5; ++i)
        targets.push_back(make_pattern(W, H, 0xAA));

    auto tp = std::make_unique<loopback_transport>();
    auto *tp_ptr = tp.get();

    streaming_session session(make_test_source(std::move(targets)), test_codecs(W, H), 6000, std::move(tp), W, H);

    // Send first 3 frames
    for (int i = 0; i < 3; ++i)
        (void)session.step();

    CHECK(session.stats().in_flight == 3);

    // Inject feedback: all 3 frames displayed
    tp_ptr->pending_feedback.push_back(make_all_displayed(3));

    // Next step will drain feedback before encoding
    (void)session.step();

    // After feedback, in_flight should be reduced (frame 4 is the only new one)
    CHECK(session.stats().in_flight == 1);
}

TEST_CASE("streaming_session — stats tracking")
{
    constexpr size_t W = 512, H = 342;
    std::vector<bitmap> targets;
    for (int i = 0; i < 3; ++i)
        targets.push_back(make_pattern(W, H, 0x55));

    auto tp = std::make_unique<loopback_transport>();

    streaming_session session(make_test_source(std::move(targets)), test_codecs(W, H), 4000, std::move(tp), W, H);

    auto s0 = session.stats();
    CHECK(s0.frames_sent == 0);
    CHECK(s0.current_byterate > 0);

    (void)session.step();
    auto s1 = session.stats();
    CHECK(s1.frames_sent == 1);
    CHECK(s1.in_flight == 1);
}

TEST_CASE("streaming_session — empty source finishes immediately")
{
    constexpr size_t W = 512, H = 342;
    auto tp = std::make_unique<loopback_transport>();
    auto *tp_ptr = tp.get();

    streaming_session session(make_test_source({}), test_codecs(W, H), 6000, std::move(tp), W, H);

    CHECK(session.step() == step_result::finished);
    CHECK(tp_ptr->sent_frames.empty());
}

TEST_CASE("streaming_session — feedback with missed frames affects rate")
{
    constexpr size_t W = 512, H = 342;
    std::vector<bitmap> targets;
    for (int i = 0; i < 20; ++i)
        targets.push_back(make_pattern(W, H, i % 2 == 0 ? 0x33 : 0xCC));

    auto tp = std::make_unique<loopback_transport>();
    auto *tp_ptr = tp.get();

    streaming_session session(make_test_source(std::move(targets)), test_codecs(W, H), 6000, std::move(tp), W, H);

    size_t initial_byterate = session.stats().current_byterate;

    // Send 10 frames
    for (int i = 0; i < 10; ++i)
        (void)session.step();

    // Report heavy loss: only every other frame displayed
    feedback_packet fb;
    fb.last_displayed_seq = 10;
    fb.history.fill(0x00);
    // Set bits for even-numbered frames only (50% loss)
    for (uint32_t seq = 1; seq <= 10; seq += 2)
    {
        size_t bit_index = 10 - seq;
        fb.history[bit_index / 8] |= (1 << (bit_index % 8));
    }
    tp_ptr->pending_feedback.push_back(fb);

    // Next step drains feedback
    (void)session.step();

    // Rate should have decreased due to heavy loss
    CHECK(session.stats().current_byterate <= initial_byterate);
}

TEST_CASE("streaming_session — process_pending_feedback standalone")
{
    constexpr size_t W = 512, H = 342;
    std::vector<bitmap> targets;
    for (int i = 0; i < 5; ++i)
        targets.push_back(make_pattern(W, H, 0xBB));

    auto tp = std::make_unique<loopback_transport>();
    auto *tp_ptr = tp.get();

    streaming_session session(make_test_source(std::move(targets)), test_codecs(W, H), 6000, std::move(tp), W, H);

    // Send 3 frames
    for (int i = 0; i < 3; ++i)
        (void)session.step();

    CHECK(session.stats().in_flight == 3);

    // Inject feedback and process it directly (not via step)
    tp_ptr->pending_feedback.push_back(make_all_displayed(2));
    session.process_pending_feedback();

    CHECK(session.stats().in_flight == 1);
}
