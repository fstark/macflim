#include "doctest.h"

#include "codec_spec.hpp"

using namespace macflim;

static constexpr size_t W = 512;
static constexpr size_t H = 342;
static constexpr size_t ROWBYTES = W / 8; // 64

// --- Helpers ---

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

// --- null ---

TEST_CASE("null: no change")
{
    auto codec = make_null_codec(W, H);
    auto current = random_bitmap(1);
    auto target = random_bitmap(2);
    bitmap before = current;

    auto data = codec->compress(current, target, 5000);

    CHECK(data.empty());
    CHECK(current == before);
}

TEST_CASE("null: any budget")
{
    auto codec = make_null_codec(W, H);
    auto current = random_bitmap(1);
    auto target = random_bitmap(2);

    CHECK(codec->compress(current, target, 0).empty());
    CHECK(codec->compress(current, target, 999999).empty());
}

// --- invert ---

TEST_CASE("invert: flips all pixels")
{
    auto codec = make_invert_codec(W, H);
    auto current = random_bitmap(42);
    bitmap before = current;

    auto data = codec->compress(current, current, 0);

    CHECK(data.empty());
    CHECK(current == before.inverted());
}

TEST_CASE("invert: double invert restores original")
{
    auto codec = make_invert_codec(W, H);
    auto current = random_bitmap(42);
    bitmap original = current;
    auto target = random_bitmap(99);

    codec->compress(current, target, 0);
    codec->compress(current, target, 0);

    CHECK(current == original);
}

// --- z32 ---

TEST_CASE("z32: blank to random improves proximity")
{
    auto codec = make_z32_codec(W, H);
    auto current = black_bitmap();
    auto target = random_bitmap(42);

    double before_prox = current.proximity(target);
    static_cast<void>(codec->compress(current, target, 6000));
    double after_prox = current.proximity(target);

    CHECK(after_prox > before_prox);
}

TEST_CASE("z32: data fits budget")
{
    auto codec = make_z32_codec(W, H);
    auto current = black_bitmap();
    auto target = random_bitmap(42);

    size_t budget = 3000;
    auto data = codec->compress(current, target, budget);

    // z32 encoded data should respect the budget (with some overhead for terminators)
    CHECK(data.size() <= budget + 16);
}

TEST_CASE("z32: zero budget produces minimal data")
{
    auto codec = make_z32_codec(W, H);
    auto current = black_bitmap();
    auto target = random_bitmap(42);
    bitmap before = current;

    auto data = codec->compress(current, target, 0);

    // With zero budget, should get at most the 4-byte terminator
    CHECK(data.size() <= 4);
    // Current should be unchanged or minimally changed
    CHECK(current.proximity(before) > 0.99);
}

TEST_CASE("z32: identical bitmaps produce minimal data")
{
    auto codec = make_z32_codec(W, H);
    auto current = random_bitmap(42);
    auto target = current;

    auto data = codec->compress(current, target, 6000);

    // Identical: only terminator expected (4 zero bytes for z32)
    CHECK(data.size() <= 4);
    CHECK(current.proximity(target) == doctest::Approx(1.0));
}

TEST_CASE("z32: random to random converges")
{
    auto codec = make_z32_codec(W, H);
    auto current = random_bitmap(1);
    auto target = random_bitmap(2);

    double prox0 = current.proximity(target);
    static_cast<void>(codec->compress(current, target, 6000));
    double prox1 = current.proximity(target);

    CHECK(prox1 > prox0);
}

TEST_CASE("z32: generous budget reaches high proximity")
{
    auto codec = make_z32_codec(W, H);
    auto current = black_bitmap();
    auto target = random_bitmap(42);

    // Very generous budget — should get close to target
    static_cast<void>(codec->compress(current, target, 100000));

    CHECK(current.proximity(target) > 0.9);
}

// --- z16 ---

TEST_CASE("z16: blank to random improves proximity")
{
    auto codec = make_z16_codec(W, H);
    auto current = black_bitmap();
    auto target = random_bitmap(42);

    double before_prox = current.proximity(target);
    static_cast<void>(codec->compress(current, target, 6000));
    double after_prox = current.proximity(target);

    CHECK(after_prox > before_prox);
}

TEST_CASE("z16: data fits budget")
{
    auto codec = make_z16_codec(W, H);
    auto current = black_bitmap();
    auto target = random_bitmap(42);

    size_t budget = 3000;
    auto data = codec->compress(current, target, budget);

    CHECK(data.size() <= budget + 16);
}

TEST_CASE("z16: zero budget produces minimal data")
{
    auto codec = make_z16_codec(W, H);
    auto current = black_bitmap();
    auto target = random_bitmap(42);
    bitmap before = current;

    auto data = codec->compress(current, target, 0);

    // z16 terminator is 2 zero bytes
    CHECK(data.size() <= 2);
    CHECK(current.proximity(before) > 0.99);
}

TEST_CASE("z16: identical bitmaps produce minimal data")
{
    auto codec = make_z16_codec(W, H);
    auto current = random_bitmap(42);
    auto target = current;

    auto data = codec->compress(current, target, 6000);

    // Identical: only terminator expected (2 zero bytes for z16)
    CHECK(data.size() <= 2);
    CHECK(current.proximity(target) == doctest::Approx(1.0));
}

TEST_CASE("z16: random to random converges")
{
    auto codec = make_z16_codec(W, H);
    auto current = random_bitmap(1);
    auto target = random_bitmap(2);

    double prox0 = current.proximity(target);
    static_cast<void>(codec->compress(current, target, 6000));
    double prox1 = current.proximity(target);

    CHECK(prox1 > prox0);
}

TEST_CASE("z16: generous budget reaches high proximity")
{
    auto codec = make_z16_codec(W, H);
    auto current = black_bitmap();
    auto target = random_bitmap(42);

    static_cast<void>(codec->compress(current, target, 100000));

    CHECK(current.proximity(target) > 0.9);
}

// --- lines ---

TEST_CASE("lines: copies scanlines and improves proximity")
{
    auto codec = make_lines_codec(W, H);
    auto current = black_bitmap();
    auto target = random_bitmap(42);

    double before_prox = current.proximity(target);
    // Budget enough for ~70 lines (70 * 64 = 4480) plus 4-byte header
    static_cast<void>(codec->compress(current, target, 70 * ROWBYTES));
    double after_prox = current.proximity(target);

    CHECK(after_prox > before_prox);
}

TEST_CASE("lines: data fits budget")
{
    auto codec = make_lines_codec(W, H);
    auto current = black_bitmap();
    auto target = random_bitmap(42);

    size_t budget = 70 * ROWBYTES;
    auto data = codec->compress(current, target, budget);

    // Data = 4-byte header + raw line bytes; raw bytes ≤ budget
    CHECK(data.size() <= budget + 4);
}

TEST_CASE("lines: generous budget copies many lines")
{
    auto codec = make_lines_codec(W, H);
    auto current = black_bitmap();
    auto target = random_bitmap(42);

    // Budget for all 342 lines
    static_cast<void>(codec->compress(current, target, H * ROWBYTES));

    CHECK(current.proximity(target) > 0.9);
}

TEST_CASE("lines: after compress, copied lines match target")
{
    auto codec = make_lines_codec(W, H);
    bitmap current(W, H);
    current.fill(0x00);
    bitmap target(W, H);
    target.fill(0xFF);

    // Budget for ~70 lines
    static_cast<void>(codec->compress(current, target, 70 * ROWBYTES));

    // Some contiguous block of lines should now be 0xFF
    auto data = current.raw_data();
    size_t matching_bytes = 0;
    for (auto byte : data)
        if (byte == 0xFF)
            matching_bytes++;

    // At least 70 * 64 bytes should be 0xFF
    CHECK(matching_bytes >= 70 * ROWBYTES);
}

// --- codec names ---

TEST_CASE("codec names are correct")
{
    CHECK(make_null_codec(W, H)->name() == "null");
    CHECK(make_invert_codec(W, H)->name() == "invert");
    CHECK(make_lines_codec(W, H)->name() == "lines");
    CHECK(make_z16_codec(W, H)->name() == "z16");
    CHECK(make_z32_codec(W, H)->name() == "z32");
}

// --- make_codec factory ---

TEST_CASE("make_codec: known codecs")
{
    auto spec = make_codec("z32", W, H);
    CHECK(spec.signature == 0x02);
    CHECK(spec.coder->name() == "z32");

    auto spec2 = make_codec("null", W, H);
    CHECK(spec2.signature == 0x00);
}

TEST_CASE("make_codec: unknown codec throws")
{
    CHECK_THROWS(static_cast<void>(make_codec("nonexistent", W, H)));
}
