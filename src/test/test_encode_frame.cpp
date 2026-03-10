#include "../doctest.h"

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

static std::vector<codec_spec> all_codecs()
{
    return {make_codec("null", W, H), make_codec("z16", W, H), make_codec("z32", W, H), make_codec("invert", W, H),
            make_codec("lines", W, H)};
}

// --- basic behavior ---

TEST_CASE("encode_frame: updates current_fb")
{
    auto current = black_bitmap();
    auto target = random_bitmap(42);
    auto codecs = all_codecs();
    bitmap before = current;

    auto result = encode_frame(current, target, codecs, 6000);
    (void)result;

    // current_fb should have been modified toward target
    CHECK(current != before);
    CHECK(current.proximity(target) > before.proximity(target));
}

TEST_CASE("encode_frame: returns best codec result")
{
    auto current = black_bitmap();
    auto target = random_bitmap(42);
    auto codecs = all_codecs();

    auto result = encode_frame(current, target, codecs, 6000);

    // The result image should match what current_fb was updated to
    CHECK(result.image() == current);
}

TEST_CASE("encode_frame: identical bitmaps give quality 1.0")
{
    auto bm = random_bitmap(42);
    auto current = bm;
    auto codecs = all_codecs();

    auto result = encode_frame(current, bm, codecs, 6000);

    CHECK(result.quality() == doctest::Approx(1.0));
    CHECK(current == bm);
}

TEST_CASE("encode_frame: respects budget")
{
    auto current = black_bitmap();
    auto target = random_bitmap(42);
    auto codecs = all_codecs();

    size_t budget = 500;
    auto result = encode_frame(current, target, codecs, budget);

    auto data = result.get_video_encoded_data();
    // Payload (minus 4-byte codec header) should fit within budget
    CHECK(data.size() - 4 <= budget + 16);
}

TEST_CASE("encode_frame: zero budget still works")
{
    auto current = black_bitmap();
    auto target = random_bitmap(42);
    auto codecs = all_codecs();

    auto result = encode_frame(current, target, codecs, 0);

    // Should not crash; null codec handles zero budget
    CHECK(result.quality() >= 0.0);
}

TEST_CASE("encode_frame: generous budget reaches high quality")
{
    auto current = black_bitmap();
    auto target = random_bitmap(42);
    auto codecs = all_codecs();

    auto result = encode_frame(current, target, codecs, 60000);

    CHECK(result.quality() > 0.9);
}

// --- consistency with encoding_result ---

TEST_CASE("encode_frame: result matches manual best-of-N")
{
    auto current = black_bitmap();
    auto target = random_bitmap(42);
    auto codecs = all_codecs();
    auto current_copy = current;

    auto result = encode_frame(current, target, codecs, 6000);

    // Manually try each codec and find the best
    double best_quality = -1.0;
    for (const auto &codec : codecs)
    {
        encoding_result er(codec, current_copy, target, 6000);
        if (er.quality() > best_quality)
            best_quality = er.quality();
    }

    CHECK(result.quality() == doctest::Approx(best_quality));
}

// --- multi-frame sequence ---

TEST_CASE("encode_frame: progressive convergence")
{
    auto current = black_bitmap();
    auto target = random_bitmap(42);
    auto codecs = all_codecs();

    double prev_quality = 0.0;
    for (int i = 0; i < 5; i++)
    {
        auto result = encode_frame(current, target, codecs, 2000);
        CHECK(result.quality() >= prev_quality);
        prev_quality = result.quality();
    }

    CHECK(prev_quality > 0.6);
}
