#include "../doctest.h"

#include "../streaming/protocol.hpp"

#include <cstdint>
#include <vector>

namespace macflim
{

// ---------------------------------------------------------------------------
// Round-trip: serialize → parse → compare
// ---------------------------------------------------------------------------

TEST_CASE("protocol: hello round-trip")
{
    hello_packet orig;
    orig.version = 1;
    orig.width = 512;
    orig.height = 342;
    orig.byterate = 6000;
    orig.dither = 2;
    orig.num_codecs = 3;
    orig.codecs = {0x02, 0x03, 0x00, 0, 0, 0, 0, 0};

    auto wire = serialize(orig);
    CHECK(wire.size() == HELLO_SIZE);

    auto parsed = parse_hello(wire.data(), wire.size());
    REQUIRE(parsed.has_value());
    CHECK(parsed->version == orig.version);
    CHECK(parsed->width == orig.width);
    CHECK(parsed->height == orig.height);
    CHECK(parsed->byterate == orig.byterate);
    CHECK(parsed->dither == orig.dither);
    CHECK(parsed->num_codecs == orig.num_codecs);
    CHECK(parsed->codecs == orig.codecs);
}

TEST_CASE("protocol: hello_ack round-trip")
{
    hello_ack_packet orig;
    orig.version = 1;
    orig.width = 512;
    orig.height = 342;
    orig.byterate = 3000;
    orig.dither = 1;
    orig.num_codecs = 2;
    orig.codecs = {0x01, 0x02, 0, 0, 0, 0, 0, 0};

    auto wire = serialize(orig);
    CHECK(wire.size() == HELLO_ACK_SIZE);

    auto parsed = parse_hello_ack(wire.data(), wire.size());
    REQUIRE(parsed.has_value());
    CHECK(parsed->version == orig.version);
    CHECK(parsed->width == orig.width);
    CHECK(parsed->height == orig.height);
    CHECK(parsed->byterate == orig.byterate);
    CHECK(parsed->dither == orig.dither);
    CHECK(parsed->num_codecs == orig.num_codecs);
    CHECK(parsed->codecs == orig.codecs);
}

TEST_CASE("protocol: frame round-trip")
{
    frame_header hdr;
    hdr.seq = 42;
    hdr.ticks = 1;
    std::vector<uint8_t> video = {0x00, 0x00, 0x00, 0x02, 0xDE, 0xAD, 0xBE, 0xEF};

    auto wire = serialize(hdr, video);
    CHECK(wire.size() == FRAME_HEADER_SIZE + video.size());

    auto parsed = parse_frame(wire.data(), wire.size());
    REQUIRE(parsed.has_value());
    CHECK(parsed->header.seq == 42);
    CHECK(parsed->header.ticks == 1);
    CHECK(parsed->video_len == video.size());
    CHECK(std::vector<uint8_t>(parsed->video_data, parsed->video_data + parsed->video_len) == video);
}

TEST_CASE("protocol: frame with empty video data")
{
    frame_header hdr;
    hdr.seq = 1;
    hdr.ticks = 1;

    auto wire = serialize(hdr, {});
    CHECK(wire.size() == FRAME_HEADER_SIZE);

    auto parsed = parse_frame(wire.data(), wire.size());
    REQUIRE(parsed.has_value());
    CHECK(parsed->header.seq == 1);
    CHECK(parsed->video_len == 0);
}

TEST_CASE("protocol: feedback round-trip")
{
    feedback_packet orig;
    orig.last_displayed_seq = 100;
    orig.history.fill(0x00);
    orig.history[0] = 0xD7; // bits: seq 100=1, 99=1, 98=1, 97=0, 96=1, 95=0, 94=1, 93=1
    orig.history[1] = 0xFF;

    auto wire = serialize(orig);
    CHECK(wire.size() == FEEDBACK_SIZE);

    auto parsed = parse_feedback(wire.data(), wire.size());
    REQUIRE(parsed.has_value());
    CHECK(parsed->last_displayed_seq == orig.last_displayed_seq);
    CHECK(parsed->history == orig.history);
}

// ---------------------------------------------------------------------------
// Wire format verification: check exact byte layout for 68K compatibility
// ---------------------------------------------------------------------------

TEST_CASE("protocol: hello wire format — big-endian, 68K-aligned")
{
    hello_packet p;
    p.version = 1;
    p.width = 512;
    p.height = 342;
    p.byterate = 6000;
    p.dither = 2;
    p.num_codecs = 2;
    p.codecs = {0x02, 0x04, 0, 0, 0, 0, 0, 0};

    auto w = serialize(p);

    //  Magic "FLMS" at offset 0
    CHECK(w[0] == 'F');
    CHECK(w[1] == 'L');
    CHECK(w[2] == 'M');
    CHECK(w[3] == 'S');

    //  Version at offset 4 (uint16 big-endian)
    CHECK(w[4] == 0x00);
    CHECK(w[5] == 0x01);

    //  Width 512 at offset 6
    CHECK(w[6] == 0x02);
    CHECK(w[7] == 0x00);

    //  Height 342 at offset 8
    CHECK(w[8] == 0x01);
    CHECK(w[9] == 0x56);

    //  Byterate 6000 at offset 10
    CHECK(w[10] == 0x17);
    CHECK(w[11] == 0x70);

    //  Dither at offset 12
    CHECK(w[12] == 0x02);

    //  Num codecs at offset 13
    CHECK(w[13] == 0x02);

    //  Codec signatures at offset 14
    CHECK(w[14] == 0x02);
    CHECK(w[15] == 0x04);
    CHECK(w[16] == 0x00); // zero-padded
}

TEST_CASE("protocol: frame wire format — big-endian, 68K-aligned")
{
    frame_header hdr;
    hdr.seq = 0x00000100; // 256
    hdr.ticks = 1;
    std::vector<uint8_t> video = {0xAA, 0xBB};

    auto w = serialize(hdr, video);

    //  Magic "FLMF" at offset 0
    CHECK(w[0] == 'F');
    CHECK(w[1] == 'L');
    CHECK(w[2] == 'M');
    CHECK(w[3] == 'F');

    //  Seq at offset 4 (uint32 big-endian)
    CHECK(w[4] == 0x00);
    CHECK(w[5] == 0x00);
    CHECK(w[6] == 0x01);
    CHECK(w[7] == 0x00);

    //  Ticks at offset 8 (uint16 big-endian)
    CHECK(w[8] == 0x00);
    CHECK(w[9] == 0x01);

    //  Video data at offset 10
    CHECK(w[10] == 0xAA);
    CHECK(w[11] == 0xBB);
}

TEST_CASE("protocol: feedback wire format — big-endian, 68K-aligned")
{
    feedback_packet p;
    p.last_displayed_seq = 0x00001234;
    p.history.fill(0x00);
    p.history[0] = 0xFF;
    p.history[15] = 0x01;

    auto w = serialize(p);

    //  Magic "FLMR" at offset 0
    CHECK(w[0] == 'F');
    CHECK(w[1] == 'L');
    CHECK(w[2] == 'M');
    CHECK(w[3] == 'R');

    //  last_displayed_seq at offset 4
    CHECK(w[4] == 0x00);
    CHECK(w[5] == 0x00);
    CHECK(w[6] == 0x12);
    CHECK(w[7] == 0x34);

    //  History bitmap at offset 8 (16 bytes, verbatim)
    CHECK(w[8] == 0xFF);
    for (int i = 9; i < 23; ++i)
        CHECK(w[i] == 0x00);
    CHECK(w[23] == 0x01);
}

// ---------------------------------------------------------------------------
// Rejection: truncated data, wrong magic
// ---------------------------------------------------------------------------

TEST_CASE("protocol: parse rejects truncated data")
{
    CHECK_FALSE(parse_hello(nullptr, 0).has_value());
    CHECK_FALSE(parse_hello_ack(nullptr, 0).has_value());
    CHECK_FALSE(parse_frame(nullptr, 0).has_value());
    CHECK_FALSE(parse_feedback(nullptr, 0).has_value());

    //  One byte short
    std::vector<uint8_t> short_hello(HELLO_SIZE - 1, 0);
    CHECK_FALSE(parse_hello(short_hello.data(), short_hello.size()).has_value());

    std::vector<uint8_t> short_feedback(FEEDBACK_SIZE - 1, 0);
    CHECK_FALSE(parse_feedback(short_feedback.data(), short_feedback.size()).has_value());

    std::vector<uint8_t> short_frame(FRAME_HEADER_SIZE - 1, 0);
    CHECK_FALSE(parse_frame(short_frame.data(), short_frame.size()).has_value());
}

TEST_CASE("protocol: parse rejects wrong magic")
{
    //  Serialize a hello, then try to parse it as hello_ack
    hello_packet hp;
    hp.width = 512;
    hp.height = 342;
    auto wire = serialize(hp);

    CHECK(parse_hello(wire.data(), wire.size()).has_value());
    CHECK_FALSE(parse_hello_ack(wire.data(), wire.size()).has_value());
    CHECK_FALSE(parse_frame(wire.data(), wire.size()).has_value());
    CHECK_FALSE(parse_feedback(wire.data(), wire.size()).has_value());
}

// ---------------------------------------------------------------------------
// peek_magic
// ---------------------------------------------------------------------------

TEST_CASE("protocol: peek_magic identifies packet types")
{
    hello_packet hp;
    auto hello_wire = serialize(hp);
    CHECK(peek_magic(hello_wire.data(), hello_wire.size()) == MAGIC_HELLO);

    feedback_packet fp;
    auto fb_wire = serialize(fp);
    CHECK(peek_magic(fb_wire.data(), fb_wire.size()) == MAGIC_FEEDBACK);

    CHECK_FALSE(peek_magic(nullptr, 0).has_value());
    uint8_t three_bytes[] = {0x46, 0x4C, 0x4D};
    CHECK_FALSE(peek_magic(three_bytes, 3).has_value());
}

// ---------------------------------------------------------------------------
// Extra data tolerance: parse should succeed with trailing bytes
// ---------------------------------------------------------------------------

TEST_CASE("protocol: parse tolerates trailing bytes")
{
    feedback_packet orig;
    orig.last_displayed_seq = 7;
    orig.history[0] = 0xFF;

    auto wire = serialize(orig);
    //  Append garbage
    wire.push_back(0xDE);
    wire.push_back(0xAD);

    auto parsed = parse_feedback(wire.data(), wire.size());
    REQUIRE(parsed.has_value());
    CHECK(parsed->last_displayed_seq == 7);
}

} // namespace macflim
