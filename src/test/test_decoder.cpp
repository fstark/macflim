#include "../doctest.h"

#include "../decoder.hpp"
#include "../encode_frame.hpp"

using namespace macflim;

static constexpr size_t W = 512;
static constexpr size_t H = 342;

static bitmap random_bitmap(int seed)
{
    bitmap b(W, H);
    b.randomize(seed);
    return b;
}

static bitmap black_bitmap()
{
    bitmap b(W, H);
    b.fill(0x00);
    return b;
}

static bitmap white_bitmap()
{
    bitmap b(W, H);
    b.fill(0xFF);
    return b;
}

// --- apply_delta basic tests ---

TEST_CASE("apply_delta: null codec leaves screen unchanged")
{
    auto screen = random_bitmap(42);
    auto before = screen;

    //  Build a null-codec encoded payload: header + no data
    std::vector<uint8_t> encoded = {0x00, 0x00, 0x00, 0x00};
    apply_delta(screen, encoded);

    CHECK(screen == before);
}

TEST_CASE("apply_delta: invert codec flips all pixels")
{
    auto screen = random_bitmap(42);
    auto inverted = screen.inverted();

    std::vector<uint8_t> encoded = {0x00, 0x00, 0x00, 0x03};
    apply_delta(screen, encoded);

    CHECK(screen == inverted);
}

TEST_CASE("apply_delta: invert twice restores original")
{
    auto screen = random_bitmap(42);
    auto original = screen;

    std::vector<uint8_t> encoded = {0x00, 0x00, 0x00, 0x03};
    apply_delta(screen, encoded);
    apply_delta(screen, encoded);

    CHECK(screen == original);
}

TEST_CASE("apply_delta: header too short throws")
{
    auto screen = random_bitmap(42);
    std::vector<uint8_t> short_data = {0x00, 0x00};

    CHECK_THROWS(apply_delta(screen, short_data));
}

TEST_CASE("apply_delta: unknown signature throws")
{
    auto screen = random_bitmap(42);
    std::vector<uint8_t> encoded = {0x00, 0x00, 0x00, 0xFF};

    CHECK_THROWS(apply_delta(screen, encoded));
}

// --- Round-trip tests: encode_frame() + apply_delta() must agree ---

//  Helper: run one round-trip test with a specific codec
static void check_round_trip(const std::string &codec_name, const bitmap &initial, const bitmap &target,
                             size_t budget)
{
    std::vector<codec_spec> codecs = {make_codec(codec_name, W, H)};

    //  Encode: modifies current_fb in place, returns the result
    auto encoder_fb = initial;
    auto result = encode_frame(encoder_fb, target, codecs, budget);

    //  Decode: apply the encoded delta to a fresh copy of initial
    auto decoder_fb = initial;
    auto encoded_data = result.get_video_encoded_data();
    apply_delta(decoder_fb, encoded_data);

    //  The decoder must produce the same bitmap as the encoder
    INFO("codec: ", codec_name, " budget: ", budget);
    CHECK(decoder_fb == encoder_fb);
}

// --- z32 round-trip ---

TEST_CASE("round-trip: z32 black to random")
{
    check_round_trip("z32", black_bitmap(), random_bitmap(42), 6000);
}

TEST_CASE("round-trip: z32 random to random")
{
    check_round_trip("z32", random_bitmap(1), random_bitmap(2), 6000);
}

TEST_CASE("round-trip: z32 tight budget")
{
    check_round_trip("z32", black_bitmap(), random_bitmap(42), 200);
}

TEST_CASE("round-trip: z32 generous budget")
{
    check_round_trip("z32", black_bitmap(), random_bitmap(42), 60000);
}

TEST_CASE("round-trip: z32 identical bitmaps")
{
    auto bm = random_bitmap(42);
    check_round_trip("z32", bm, bm, 6000);
}

// --- z16 round-trip ---

TEST_CASE("round-trip: z16 black to random")
{
    check_round_trip("z16", black_bitmap(), random_bitmap(42), 6000);
}

TEST_CASE("round-trip: z16 random to random")
{
    check_round_trip("z16", random_bitmap(1), random_bitmap(2), 6000);
}

TEST_CASE("round-trip: z16 tight budget")
{
    check_round_trip("z16", black_bitmap(), random_bitmap(42), 200);
}

TEST_CASE("round-trip: z16 identical bitmaps")
{
    auto bm = random_bitmap(42);
    check_round_trip("z16", bm, bm, 6000);
}

// --- lines round-trip ---

TEST_CASE("round-trip: lines black to white")
{
    check_round_trip("lines", black_bitmap(), white_bitmap(), 6000);
}

TEST_CASE("round-trip: lines black to random")
{
    check_round_trip("lines", black_bitmap(), random_bitmap(42), 6000);
}

TEST_CASE("round-trip: lines generous budget")
{
    check_round_trip("lines", black_bitmap(), random_bitmap(42), 60000);
}

// --- invert round-trip ---

TEST_CASE("round-trip: invert black to white")
{
    check_round_trip("invert", black_bitmap(), white_bitmap(), 6000);
}

// --- null round-trip ---

TEST_CASE("round-trip: null codec")
{
    check_round_trip("null", random_bitmap(1), random_bitmap(2), 6000);
}

// --- Multi-codec encode_frame + apply_delta ---

TEST_CASE("round-trip: all codecs, best selection")
{
    std::vector<codec_spec> codecs = {make_codec("null", W, H), make_codec("z16", W, H),
                                      make_codec("z32", W, H), make_codec("invert", W, H),
                                      make_codec("lines", W, H)};

    auto encoder_fb = black_bitmap();
    auto target = random_bitmap(42);

    auto result = encode_frame(encoder_fb, target, codecs, 6000);

    auto decoder_fb = black_bitmap();
    apply_delta(decoder_fb, result.get_video_encoded_data());

    CHECK(decoder_fb == encoder_fb);
}

TEST_CASE("round-trip: progressive multi-frame convergence")
{
    std::vector<codec_spec> codecs = {make_codec("null", W, H), make_codec("z16", W, H),
                                      make_codec("z32", W, H), make_codec("invert", W, H),
                                      make_codec("lines", W, H)};

    auto target = random_bitmap(42);

    //  Both encoder and decoder start from the same initial screen
    auto encoder_fb = black_bitmap();
    auto decoder_fb = black_bitmap();

    //  Simulate several frames of progressive encoding
    for (int i = 0; i < 5; i++)
    {
        auto result = encode_frame(encoder_fb, target, codecs, 2000);
        apply_delta(decoder_fb, result.get_video_encoded_data());

        INFO("frame: ", i);
        CHECK(decoder_fb == encoder_fb);
    }

    //  After several frames, should be close to target
    CHECK(encoder_fb.proximity(target) > 0.5);
}
