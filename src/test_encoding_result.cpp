#include "doctest.h"

#include "encoding_result.hpp"

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

// --- codec header ---

TEST_CASE("encoding_result: codec header")
{
    codec_spec spec = make_codec("z32", W, H);
    auto current = black_bitmap();
    auto target = random_bitmap(42);

    encoding_result er(spec, current, target, 6000);
    auto data = er.get_video_encoded_data();

    REQUIRE(data.size() >= 4);
    CHECK(data[0] == 0x00);
    CHECK(data[1] == 0x00);
    CHECK(data[2] == 0x00);
    CHECK(data[3] == spec.signature);
}

TEST_CASE("encoding_result: null codec header")
{
    codec_spec spec = make_codec("null", W, H);
    auto current = random_bitmap(1);
    auto target = random_bitmap(2);

    encoding_result er(spec, current, target, 6000);
    auto data = er.get_video_encoded_data();

    REQUIRE(data.size() >= 4);
    CHECK(data[3] == 0x00); // null signature
}

// --- quality metric ---

TEST_CASE("encoding_result: quality matches proximity")
{
    codec_spec spec = make_codec("z32", W, H);
    auto current = black_bitmap();
    auto target = random_bitmap(42);

    encoding_result er(spec, current, target, 6000);

    // quality() should equal proximity(result image, target)
    double expected = er.image().proximity(target);
    CHECK(er.quality() == doctest::Approx(expected));
}

TEST_CASE("encoding_result: identical bitmaps give quality 1.0")
{
    codec_spec spec = make_codec("null", W, H);
    auto bm = random_bitmap(42);

    encoding_result er(spec, bm, bm, 6000);

    CHECK(er.quality() == doctest::Approx(1.0));
}

// --- best-of-N selection ---

TEST_CASE("encoding_result: best-of-N selection")
{
    auto current = black_bitmap();
    auto target = random_bitmap(42);

    // null won't change current, so quality is low
    codec_spec null_spec = make_codec("null", W, H);
    encoding_result null_er(null_spec, current, target, 6000);

    // z32 with generous budget should produce higher quality
    codec_spec z32_spec = make_codec("z32", W, H);
    encoding_result z32_er(z32_spec, current, target, 6000);

    // z32 should beat null for a different target
    CHECK(z32_er.quality() > null_er.quality());
}

// --- budget respected ---

TEST_CASE("encoding_result: budget respected")
{
    codec_spec spec = make_codec("z32", W, H);
    auto current = black_bitmap();
    auto target = random_bitmap(42);

    size_t budget = 3000;
    encoding_result er(spec, current, target, budget);

    auto data = er.get_video_encoded_data();
    // Encoded data minus 4-byte header should fit within budget * penalty (+ small overhead)
    size_t payload = data.size() - 4;
    CHECK(payload <= static_cast<size_t>(budget * spec.penalty) + 16);
}
