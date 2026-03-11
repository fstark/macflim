#include "../doctest.h"
#include "../ruler.hpp"

#include <format>
#include <iostream>
#include <limits>
#include <random>

namespace macflim
{

TEST_CASE("uint8_ruler - identical values")
{
    const uint8_ruler &ruler = uint8_ruler::ruler;

    // Distance from any value to itself should be zero
    for (int i = 0; i < 256; i++)
    {
        CHECK(ruler.distance(i, i) == 0);
    }
}

TEST_CASE("uint8_ruler - symmetry")
{
    const uint8_ruler &ruler = uint8_ruler::ruler;

    // Distance should be symmetric: d(a,b) == d(b,a)
    CHECK(ruler.distance(0x00, 0xFF) == ruler.distance(0xFF, 0x00));
    CHECK(ruler.distance(0x12, 0x34) == ruler.distance(0x34, 0x12));
    CHECK(ruler.distance(0x00, 0x01) == ruler.distance(0x01, 0x00));
}

TEST_CASE("uint8_ruler - single bit flip")
{
    const uint8_ruler &ruler = uint8_ruler::ruler;

    // Flipping a single bit should have distance 2
    CHECK(ruler.distance(0b00000000, 0b00000001) == 2);
    CHECK(ruler.distance(0b00000000, 0b00000010) == 2);
    CHECK(ruler.distance(0b00000000, 0b00000100) == 2);
    CHECK(ruler.distance(0b00000000, 0b10000000) == 2);
    CHECK(ruler.distance(0b11111111, 0b11111110) == 2);
    CHECK(ruler.distance(0b11111111, 0b01111111) == 2);
}

TEST_CASE("uint8_ruler - adjacent bit swap")
{
    const uint8_ruler &ruler = uint8_ruler::ruler;

    // Swapping adjacent bits - the actual distances are computed by the ruler algorithm
    auto d1 = ruler.distance(0b00000000, 0b00000011);
    auto d2 = ruler.distance(0b00000100, 0b00001000);
    auto d3 = ruler.distance(0b00110000, 0b01100000);
    auto d4 = ruler.distance(0b11111111, 0b11111100);

    // Just verify they have some distance (actual values depend on algorithm)
    CHECK(d1 > 0);
    CHECK(d2 > 0);
    CHECK(d3 > 0);
    CHECK(d4 > 0);

    // Adjacent swaps should have less distance than arbitrary changes
    auto d_arbitrary = ruler.distance(0b00000000, 0b11111111);
    CHECK(d1 < d_arbitrary);
    CHECK(d2 < d_arbitrary);
}

TEST_CASE("uint8_ruler - multiple bit changes")
{
    const uint8_ruler &ruler = uint8_ruler::ruler;

    // Two separate bit flips: distance should be 2+2 = 4 (or optimized path)
    auto d1 = ruler.distance(0b00000000, 0b00000101); // bits 0 and 2 flipped
    CHECK(d1 > 0);

    // All bits different
    auto d2 = ruler.distance(0x00, 0xFF);
    CHECK(d2 > 0);

    // Check that distance increases with more differences
    CHECK(ruler.distance(0b00000000, 0b00000001) < ruler.distance(0b00000000, 0b00010001));
}

TEST_CASE("uint8_ruler - triangle inequality")
{
    const uint8_ruler &ruler = uint8_ruler::ruler;

    // For any three values a, b, c: d(a,c) <= d(a,b) + d(b,c)
    uint8_t a = 0b00000000;
    uint8_t b = 0b00001111;
    uint8_t c = 0b11111111;

    auto d_ac = ruler.distance(a, c);
    auto d_ab = ruler.distance(a, b);
    auto d_bc = ruler.distance(b, c);

    CHECK(d_ac <= d_ab + d_bc);

    // Another example
    a = 0x00;
    b = 0x0F;
    c = 0xFF;

    d_ac = ruler.distance(a, c);
    d_ab = ruler.distance(a, b);
    d_bc = ruler.distance(b, c);

    CHECK(d_ac <= d_ab + d_bc);
}

TEST_CASE("uint8_ruler - known specific distances")
{
    const uint8_ruler &ruler = uint8_ruler::ruler;

    // Some specific expected behaviors based on the algorithm
    // These verify the ruler is working consistently
    auto d1 = ruler.distance(0x00, 0x00);
    auto d2 = ruler.distance(0xFF, 0xFF);
    CHECK(d1 == 0);
    CHECK(d2 == 0);

    // Single bit flips have distance 2
    CHECK(ruler.distance(0b00000000, 0b00000001) == 2);
    CHECK(ruler.distance(0b00000001, 0b00000011) == 2);

    // Verify distances are positive for different values
    CHECK(ruler.distance(0b00, 0b11) > 0);
    CHECK(ruler.distance(0b01, 0b10) > 0);
}

TEST_CASE("uint8_ruler - legacy specific distance values")
{
    const uint8_ruler &ruler = uint8_ruler::ruler;

    // Precise distance values from legacy ruler.cpp tests
    CHECK(ruler.distance(0b00000001, 0b00000001) == 0);
    CHECK(ruler.distance(0b10000001, 0b00000001) == 2);
    CHECK(ruler.distance(0b10000001, 0b00000000) == 4);
    CHECK(ruler.distance(0b11111111, 0b00000000) == 16);

    // Adjacent bit swaps have distance 1
    CHECK(ruler.distance(0b00000001, 0b00000010) == 1);
    CHECK(ruler.distance(0b00000001, 0b00000100) == 2);
    CHECK(ruler.distance(0b00000001, 0b00001000) == 3);
}

TEST_CASE("uint16_ruler - identical values")
{
    const uint16_ruler &ruler = uint16_ruler::ruler;

    // Sample identical values should have zero distance
    CHECK(ruler.distance(0x0000, 0x0000) == 0);
    CHECK(ruler.distance(0x1234, 0x1234) == 0);
    CHECK(ruler.distance(0xFFFF, 0xFFFF) == 0);
}

TEST_CASE("uint16_ruler - symmetry")
{
    const uint16_ruler &ruler = uint16_ruler::ruler;

    // Distance should be symmetric
    CHECK(ruler.distance(0x0000, 0xFFFF) == ruler.distance(0xFFFF, 0x0000));
    CHECK(ruler.distance(0x1234, 0x5678) == ruler.distance(0x5678, 0x1234));
}

TEST_CASE("uint16_ruler - byte composition")
{
    const uint16_ruler &ruler = uint16_ruler::ruler;
    const uint8_ruler &byte_ruler = uint8_ruler::ruler;

    // uint16 distance should be sum of high and low byte distances
    uint16_t a = 0x1234;
    uint16_t b = 0x5678;

    auto d16 = ruler.distance(a, b);
    auto d_high = byte_ruler.distance(0x12, 0x56);
    auto d_low = byte_ruler.distance(0x34, 0x78);

    CHECK(d16 == d_high + d_low);
}

TEST_CASE("uint16_ruler - single byte difference")
{
    const uint16_ruler &ruler = uint16_ruler::ruler;

    // Only high byte differs
    auto d1 = ruler.distance(0x1234, 0x5634);
    CHECK(d1 > 0);

    // Only low byte differs
    auto d2 = ruler.distance(0x1234, 0x1278);
    CHECK(d2 > 0);

    // Distance when one byte unchanged should be less than when both differ
    auto d3 = ruler.distance(0x1234, 0x5678);
    CHECK(d1 < d3);
    CHECK(d2 < d3);
}

TEST_CASE("uint32_ruler - identical values")
{
    const uint32_ruler &ruler = uint32_ruler::ruler;

    CHECK(ruler.distance(0x00000000, 0x00000000) == 0);
    CHECK(ruler.distance(0x12345678, 0x12345678) == 0);
    CHECK(ruler.distance(0xFFFFFFFF, 0xFFFFFFFF) == 0);
}

TEST_CASE("uint32_ruler - symmetry")
{
    const uint32_ruler &ruler = uint32_ruler::ruler;

    CHECK(ruler.distance(0x00000000, 0xFFFFFFFF) == ruler.distance(0xFFFFFFFF, 0x00000000));
    CHECK(ruler.distance(0x12345678, 0x9ABCDEF0) == ruler.distance(0x9ABCDEF0, 0x12345678));
}

TEST_CASE("uint32_ruler - byte composition")
{
    const uint32_ruler &ruler = uint32_ruler::ruler;
    const uint8_ruler &byte_ruler = uint8_ruler::ruler;

    // uint32 distance should be sum of all four byte distances
    uint32_t a = 0x12345678;
    uint32_t b = 0x9ABCDEF0;

    auto d32 = ruler.distance(a, b);
    auto d0 = byte_ruler.distance(0x12, 0x9A);
    auto d1 = byte_ruler.distance(0x34, 0xBC);
    auto d2 = byte_ruler.distance(0x56, 0xDE);
    auto d3 = byte_ruler.distance(0x78, 0xF0);

    CHECK(d32 == d0 + d1 + d2 + d3);
}

TEST_CASE("uint32_ruler - partial byte differences")
{
    const uint32_ruler &ruler = uint32_ruler::ruler;

    // Only one byte differs
    auto d1 = ruler.distance(0x12345678, 0x12345600);
    CHECK(d1 > 0);

    // Two bytes differ
    auto d2 = ruler.distance(0x12345678, 0x12340000);
    CHECK(d2 > d1);

    // All bytes differ
    auto d3 = ruler.distance(0x00000000, 0xFFFFFFFF);
    CHECK(d3 > d2);
}

TEST_CASE("bit_ruler - uint8_t")
{
    bit_ruler<uint8_t> ruler;

    SUBCASE("identical values")
    {
        CHECK(ruler.distance(0x00, 0x00) == 0);
        CHECK(ruler.distance(0xFF, 0xFF) == 0);
        CHECK(ruler.distance(0xAA, 0xAA) == 0);
    }

    SUBCASE("single bit difference")
    {
        // XOR gives 1 bit set, and the multi-scale distance
        auto d = ruler.distance(0b00000000, 0b00000001);
        CHECK(d >= 1); // At least the popcount
    }

    SUBCASE("all bits different")
    {
        // XOR gives all bits set
        auto d = ruler.distance(0x00, 0xFF);
        CHECK(d >= 8); // At least the popcount of 8 bits
    }

    SUBCASE("alternating patterns")
    {
        // 0xAA = 10101010, 0x55 = 01010101
        auto d = ruler.distance(0xAA, 0x55);
        CHECK(d >= 8); // All 8 bits differ
    }
}

TEST_CASE("bit_ruler - uint16_t")
{
    bit_ruler<uint16_t> ruler;

    CHECK(ruler.distance(0x0000, 0x0000) == 0);
    CHECK(ruler.distance(0xFFFF, 0xFFFF) == 0);

    // Specific distance values (from legacy ruler.cpp tests)
    CHECK(ruler.distance(1, 0) == 5);
    CHECK(ruler.distance(0b0000100000000000, 0b1000000000000000) == 6);
    CHECK(ruler.distance(0b0000100000000000, 0b0001000000000000) == 6);
    CHECK(ruler.distance(0b0010000000000000, 0b1000000000000000) == 4);

    auto d2 = ruler.distance(0x0000, 0xFFFF);
    CHECK(d2 >= 16); // All 16 bits differ
}

TEST_CASE("bit_ruler - uint32_t")
{
    bit_ruler<uint32_t> ruler;

    CHECK(ruler.distance(0x00000000, 0x00000000) == 0);
    CHECK(ruler.distance(0xFFFFFFFF, 0xFFFFFFFF) == 0);

    // Specific distance values (from legacy ruler.cpp tests)
    CHECK(ruler.distance(1, 0) == 6);
    CHECK(ruler.distance(0b10000000000000000000000000000001, 0b00000000000000000000000000000000) == 12);
    CHECK(ruler.distance(0b10100000000000000000000000000000, 0b00000000000000000000000000000000) == 12);
    CHECK(ruler.distance(0b00100000000000000000000000000000, 0b10000000000000000000000000000000) == 4);

    auto d2 = ruler.distance(0x00000000, 0xFFFFFFFF);
    CHECK(d2 >= 32); // All 32 bits differ
}

TEST_CASE("uint32_ruler - solid vs noise vs noise-to-noise distances")
{
    const uint32_ruler &ruler = uint32_ruler::ruler;

    //  Use a fixed seed for reproducibility
    std::mt19937 rng(42);
    std::uniform_int_distribution<uint32_t> dist(0, 0xFFFFFFFF);

    constexpr size_t N = 1000;

    size_t solid_to_noise_sum = 0;
    size_t solid_to_noise_max = 0;
    size_t solid_to_noise_min = std::numeric_limits<size_t>::max();

    size_t noise_to_noise_sum = 0;
    size_t noise_to_noise_max = 0;
    size_t noise_to_noise_min = std::numeric_limits<size_t>::max();

    for (size_t i = 0; i < N; i++)
    {
        uint32_t noise_a = dist(rng);
        uint32_t noise_b = dist(rng);

        //  0xFFFFFFFF (solid black) → random noise
        size_t d1 = ruler.distance(0xFFFFFFFF, noise_a);
        solid_to_noise_sum += d1;
        solid_to_noise_max = std::max(solid_to_noise_max, d1);
        solid_to_noise_min = std::min(solid_to_noise_min, d1);

        //  random noise → different random noise
        size_t d2 = ruler.distance(noise_a, noise_b);
        noise_to_noise_sum += d2;
        noise_to_noise_max = std::max(noise_to_noise_max, d2);
        noise_to_noise_min = std::min(noise_to_noise_min, d2);
    }

    //  Print the results
    MESSAGE(std::format("solid→noise:  avg={}, min={}, max={}", solid_to_noise_sum / N, solid_to_noise_min,
                        solid_to_noise_max));
    MESSAGE(std::format("noise→noise:  avg={}, min={}, max={}", noise_to_noise_sum / N, noise_to_noise_min,
                        noise_to_noise_max));

    //  Solid-to-noise should be significantly higher than noise-to-noise on average
    //  because solid has all bits the same — changing to noise requires many flips.
    //  Noise-to-noise changes roughly half the bits, but many can swap cheaply.
    CHECK(solid_to_noise_sum > noise_to_noise_sum);
}

} // namespace macflim
