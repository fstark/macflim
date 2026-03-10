#include "../doctest.h"

#include "../bitmap.hpp"
#include "../grayscale.hpp"

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

// --- as_image ---

TEST_CASE("bitmap as_image: converts to grayscale")
{
    bitmap b(8, 2);
    b.fill(0x00); // all bits 0 = all black pixels

    auto gray = b.as_image();
    CHECK(gray.W() == 8);
    CHECK(gray.H() == 2);

    // All pixels should be white (1) because !(0) = 1
    // The as_image function inverts: bit 0 -> pixel 1, bit 1 -> pixel 0
    for (size_t y = 0; y < 2; y++)
        for (size_t x = 0; x < 8; x++)
            CHECK(gray.at(x, y) == 1);
}

TEST_CASE("bitmap as_image: white pixels")
{
    bitmap b(8, 2);
    b.fill(0xFF); // all bits 1 = all white pixels (in bitmap representation)

    auto gray = b.as_image();

    // All pixels should be black (0) because !(1) = 0
    for (size_t y = 0; y < 2; y++)
        for (size_t x = 0; x < 8; x++)
            CHECK(gray.at(x, y) == 0);
}

TEST_CASE("bitmap as_image: pattern")
{
    bitmap b(8, 1);
    // Set pattern: 10101010 (0xAA) - alternating bits
    b.fill(0xAA);

    auto gray = b.as_image();

    // Pattern is inverted: bit 1 -> pixel 0, bit 0 -> pixel 1
    CHECK(gray.at(0, 0) == 0); // bit 7 is 1
    CHECK(gray.at(1, 0) == 1); // bit 6 is 0
    CHECK(gray.at(2, 0) == 0); // bit 5 is 1
    CHECK(gray.at(3, 0) == 1); // bit 4 is 0
    CHECK(gray.at(4, 0) == 0); // bit 3 is 1
    CHECK(gray.at(5, 0) == 1); // bit 2 is 0
    CHECK(gray.at(6, 0) == 0); // bit 1 is 1
    CHECK(gray.at(7, 0) == 1); // bit 0 is 0
}

// --- extract ---

TEST_CASE("bitmap extract: single byte from uniform bitmap")
{
    bitmap b(16, 2);
    b.fill(0xAB);

    std::vector<uint8_t> extracted;
    b.extract(std::back_inserter(extracted), 0, 0, 1);

    REQUIRE(extracted.size() == 1);
    CHECK(extracted[0] == 0xAB);
}

TEST_CASE("bitmap extract: multiple bytes from uniform bitmap")
{
    bitmap b(32, 2);
    b.fill(0x42);

    std::vector<uint8_t> extracted;
    b.extract(std::back_inserter(extracted), 0, 0, 4);

    REQUIRE(extracted.size() == 4);
    for (auto byte : extracted)
        CHECK(byte == 0x42);
}

TEST_CASE("bitmap extract: from different positions")
{
    bitmap b(32, 4); // 4 bytes per row
    b.fill(0xF0);

    // Extract from start of first row
    std::vector<uint8_t> extracted1;
    b.extract(std::back_inserter(extracted1), 0, 0, 2);
    REQUIRE(extracted1.size() == 2);
    CHECK(extracted1[0] == 0xF0);
    CHECK(extracted1[1] == 0xF0);

    // Extract from middle of row
    std::vector<uint8_t> extracted2;
    b.extract(std::back_inserter(extracted2), 2, 0, 2);
    REQUIRE(extracted2.size() == 2);
    CHECK(extracted2[0] == 0xF0);
    CHECK(extracted2[1] == 0xF0);
}

TEST_CASE("bitmap extract: from second row")
{
    bitmap b(16, 4); // 2 bytes per row

    // Fill entire bitmap with 0x00, then copy a different pattern to row 1
    b.fill(0x00);

    bitmap source(16, 4);
    source.fill(0xAB);

    // Copy line 1 from source to b (copy 1 line starting at line 1)
    b.copy_lines_from(source, 1, 1);

    // Extract from row 1
    std::vector<uint8_t> extracted;
    b.extract(std::back_inserter(extracted), 0, 1, 2);

    REQUIRE(extracted.size() == 2);
    CHECK(extracted[0] == 0xAB);
    CHECK(extracted[1] == 0xAB);
}
