#include "doctest.h"

#include "bitmap.hpp"

using namespace macflim;

// --- Construction ---

TEST_CASE("bitmap construction: default fill")
{
    bitmap b(512, 342);
    auto data = b.raw_data();
    for (auto byte : data)
        CHECK(byte == 0xf0);
}

TEST_CASE("bitmap construction: dimensions")
{
    bitmap b(512, 342);
    CHECK(b.W() == 512);
    CHECK(b.H() == 342);
}

// --- Equality ---

TEST_CASE("bitmap operator==: identical")
{
    bitmap a(512, 342);
    CHECK(a == a);

    bitmap b = a;
    CHECK(a == b);
}

TEST_CASE("bitmap operator==: different content")
{
    bitmap a(512, 342);
    bitmap b(512, 342);
    b.fill(0x00);
    CHECK_FALSE(a == b);
}

// --- XOR ---

TEST_CASE("bitmap operator^: self XOR is zero")
{
    bitmap a(512, 342);
    a.randomize(42);
    bitmap z = a ^ a;
    for (auto byte : z.raw_data())
        CHECK(byte == 0x00);
}

TEST_CASE("bitmap operator^: different bitmaps")
{
    bitmap a(512, 342);
    a.fill(0xFF);
    bitmap b(512, 342);
    b.fill(0x00);
    bitmap x = a ^ b;
    for (auto byte : x.raw_data())
        CHECK(byte == 0xFF);
}

// --- Fill ---

TEST_CASE("bitmap fill")
{
    bitmap b(512, 342);
    b.fill(0x00);
    for (auto byte : b.raw_data())
        CHECK(byte == 0x00);

    b.fill(0xFF);
    for (auto byte : b.raw_data())
        CHECK(byte == 0xFF);
}

// --- Randomize ---

TEST_CASE("bitmap randomize: deterministic")
{
    bitmap a(512, 342);
    a.randomize(42);
    bitmap b(512, 342);
    b.randomize(42);
    CHECK(a == b);
}

TEST_CASE("bitmap randomize: different seeds differ")
{
    bitmap a(512, 342);
    a.randomize(1);
    bitmap b(512, 342);
    b.randomize(2);
    CHECK_FALSE(a == b);
}

// --- Invert ---

TEST_CASE("bitmap invert: double invert is identity")
{
    bitmap b(512, 342);
    b.randomize(42);
    bitmap original = b;
    CHECK(b.inverted().inverted() == original);
}

TEST_CASE("bitmap invert: inverted differs from original")
{
    bitmap b(512, 342);
    b.randomize(42);
    CHECK_FALSE(b.inverted() == b);
}

// --- Proximity ---

TEST_CASE("bitmap proximity: identical is 1.0")
{
    bitmap a(512, 342);
    a.randomize(42);
    CHECK(a.proximity(a) == doctest::Approx(1.0));
}

TEST_CASE("bitmap proximity: inverted is near 0.0")
{
    bitmap b(512, 342);
    b.randomize(42);
    // Random bitmap inverted: every bit flipped → proximity = 0.0
    CHECK(b.proximity(b.inverted()) == doctest::Approx(0.0));
}

TEST_CASE("bitmap proximity: different fill")
{
    bitmap a(512, 342);
    a.fill(0xF0);
    bitmap b(512, 342);
    b.fill(0x00);
    // 0xF0 vs 0x00: 4 bits differ per byte → 50% different
    CHECK(a.proximity(b) == doctest::Approx(0.5));
}

// --- count_differences ---

TEST_CASE("bitmap count_differences: known pattern")
{
    bitmap a(512, 342);
    a.fill(0xFF);
    bitmap b(512, 342);
    b.fill(0x00);
    // Every pixel differs: 512 * 342 = 175104
    CHECK(a.count_differences(b) == 512 * 342);
}

TEST_CASE("bitmap count_differences: identical is zero")
{
    bitmap a(512, 342);
    a.randomize(99);
    CHECK(a.count_differences(a) == 0);
}

// --- copy_lines_from ---

TEST_CASE("bitmap copy_lines_from: partial copy")
{
    bitmap a(512, 342);
    a.fill(0x00);
    bitmap b(512, 342);
    b.fill(0xFF);

    a.copy_lines_from(b, 100, 11);

    auto data = a.raw_data();
    size_t rowbytes = 512 / 8;

    // Lines before 100 are still 0x00
    for (size_t i = 0; i < 100 * rowbytes; i++)
        CHECK(data[i] == 0x00);

    // Lines 100–110 are 0xFF (copied from b)
    for (size_t i = 100 * rowbytes; i < 111 * rowbytes; i++)
        CHECK(data[i] == 0xFF);

    // Lines after 110 are still 0x00
    for (size_t i = 111 * rowbytes; i < data.size(); i++)
        CHECK(data[i] == 0x00);
}

// --- raw_values round-trips ---

TEST_CASE("bitmap raw_values round-trip uint16")
{
    bitmap original(512, 342);
    original.randomize(77);

    auto values = original.raw_values<uint16_t>();
    bitmap reconstructed(values, 512, 342);

    CHECK(original == reconstructed);
}

TEST_CASE("bitmap raw_values round-trip uint32")
{
    bitmap original(512, 342);
    original.randomize(77);

    auto values = original.raw_values<uint32_t>();
    bitmap reconstructed(values, 512, 342);

    CHECK(original == reconstructed);
}

TEST_CASE("bitmap raw_values round-trip uint8")
{
    bitmap original(512, 342);
    original.randomize(77);

    auto values = original.raw_values<uint8_t>();
    bitmap reconstructed(values, 512, 342);

    CHECK(original == reconstructed);
}

// --- raw_values vertical packing order ---

TEST_CASE("bitmap raw_values: vertical packing order")
{
    // Verify that raw_values<T>() returns column-major data:
    // All H values from the first T-width column, then next column, etc.
    bitmap b(512, 342);
    b.fill(0x00);

    // Set line 0 to all 0xFF — in horizontal layout, that's the first 64 bytes
    bitmap src(512, 342);
    src.fill(0xFF);
    b.copy_lines_from(src, 0, 1);

    auto vals32 = b.raw_values<uint32_t>();
    // With vertical packing: each column of 342 uint32_t values
    // Column 0 has val[0]=first 4 bytes of line 0 (0xFFFFFFFF), val[1..341]=0
    size_t columns = 512 / 32; // 16 columns of uint32_t
    CHECK(vals32.size() == columns * 342);

    // First value of each column should be non-zero (from line 0)
    for (size_t col = 0; col < columns; col++)
        CHECK(vals32[col * 342] == 0xFFFFFFFF);

    // Second value of each column should be zero (line 1 is empty)
    for (size_t col = 0; col < columns; col++)
        CHECK(vals32[col * 342 + 1] == 0x00000000);
}

// --- pixel_count ---

TEST_CASE("bitmap pixel_count: all ones")
{
    bitmap b(512, 342);
    b.fill(0xFF);
    CHECK(b.pixel_count() == 512 * 342);
}

TEST_CASE("bitmap pixel_count: all zeros")
{
    bitmap b(512, 342);
    b.fill(0x00);
    CHECK(b.pixel_count() == 0);
}

TEST_CASE("bitmap pixel_count: half pattern")
{
    bitmap b(512, 342);
    b.fill(0xF0); // 4 bits set per byte
    CHECK(b.pixel_count() == 512 * 342 / 2);
}
