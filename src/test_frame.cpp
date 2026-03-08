#include "doctest.h"

#include "frame.hpp"

using namespace macflim;

static constexpr size_t W = 512;
static constexpr size_t H = 342;

// Helper: serialize a frame then deserialize it
static frame round_trip(const frame &f)
{
    std::vector<uint8_t> buf;
    f.serialize(buf);
    return frame::deserialize(buf.data(), buf.size());
}

// --- round-trip with audio ---

TEST_CASE("frame round-trip: with audio")
{
    std::vector<uint8_t> video = {0x00, 0x00, 0x00, 0x02, 0xAA, 0xBB};
    std::vector<uint8_t> audio(370, 0x80); // 1 tick worth of audio
    bitmap src(W, H);
    bitmap res(W, H);

    frame f(src, 1, video, audio, res);
    auto f2 = round_trip(f);

    CHECK(f2.ticks == 1);
    CHECK(f2.video == video);
    CHECK(f2.audio == audio);
}

// --- round-trip silent ---

TEST_CASE("frame round-trip: silent")
{
    std::vector<uint8_t> video = {0x00, 0x00, 0x00, 0x02};
    std::vector<uint8_t> audio; // empty
    bitmap src(W, H);
    bitmap res(W, H);

    frame f(src, 1, video, audio, res);
    auto f2 = round_trip(f);

    CHECK(f2.ticks == 1);
    CHECK(f2.audio.empty());
    CHECK(f2.video == video);
}

// --- round-trip empty video ---

TEST_CASE("frame round-trip: empty video")
{
    std::vector<uint8_t> video; // empty
    std::vector<uint8_t> audio(370, 0x42);
    bitmap src(W, H);
    bitmap res(W, H);

    frame f(src, 1, video, audio, res);
    auto f2 = round_trip(f);

    CHECK(f2.ticks == 1);
    CHECK(f2.video.empty());
    CHECK(f2.audio == audio);
}

// --- round-trip various tick counts ---

TEST_CASE("frame round-trip: various tick counts")
{
    bitmap src(W, H);
    bitmap res(W, H);

    for (size_t ticks : {1, 4, 60})
    {
        CAPTURE(ticks);
        std::vector<uint8_t> video = {0x00, 0x01};
        std::vector<uint8_t> audio(ticks * 370, 0x55);

        frame f(src, ticks, video, audio, res);
        auto f2 = round_trip(f);

        CHECK(f2.ticks == ticks);
        CHECK(f2.video == video);
        CHECK(f2.audio.size() == audio.size());
    }
}

// --- deserialize truncated ---

TEST_CASE("frame deserialize: truncated input")
{
    // Less than 4 bytes: should return default frame without crashing
    uint8_t tiny[] = {0x01, 0x02};
    auto f = frame::deserialize(tiny, 2);
    CHECK(f.ticks == 0);
    CHECK(f.video.empty());
    CHECK(f.audio.empty());
}

TEST_CASE("frame deserialize: just enough for header")
{
    // 4 bytes: ticks=1, sound_size=2 (empty sound), then no video
    uint8_t data[] = {0x00, 0x01, 0x00, 0x02};
    auto f = frame::deserialize(data, 4);
    CHECK(f.ticks == 1);
    CHECK(f.audio.empty());
    // No video data available
    CHECK(f.video.empty());
}

// --- serialize produces valid data ---

TEST_CASE("frame serialize: produces non-empty output")
{
    bitmap src(W, H);
    bitmap res(W, H);
    std::vector<uint8_t> video = {0xAA};
    std::vector<uint8_t> audio(370, 0x80);

    frame f(src, 1, video, audio, res);
    std::vector<uint8_t> buf;
    f.serialize(buf);

    CHECK(buf.size() > 0);
    // First two bytes are ticks
    CHECK(buf[0] == 0x00);
    CHECK(buf[1] == 0x01);
}
