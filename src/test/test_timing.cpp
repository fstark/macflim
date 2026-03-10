#include "../common.hpp"
#include "../constants.hpp"
#include "../doctest.h"

namespace macflim
{

TEST_CASE("ticks_from_frame - standard frame rates")
{
    SUBCASE("30 fps")
    {
        CHECK(ticks_from_frame(0, 30.0) == 0);
        CHECK(ticks_from_frame(1, 30.0) == 2);    // 1/30 * 60 = 2
        CHECK(ticks_from_frame(30, 30.0) == 60);  // 1 second = 60 ticks
        CHECK(ticks_from_frame(60, 30.0) == 120); // 2 seconds = 120 ticks
    }

    SUBCASE("60 fps")
    {
        CHECK(ticks_from_frame(0, 60.0) == 0);
        CHECK(ticks_from_frame(1, 60.0) == 1);     // 1/60 * 60 = 1
        CHECK(ticks_from_frame(60, 60.0) == 60);   // 1 second = 60 ticks
        CHECK(ticks_from_frame(120, 60.0) == 120); // 2 seconds = 120 ticks
    }

    SUBCASE("15 fps")
    {
        CHECK(ticks_from_frame(0, 15.0) == 0);
        CHECK(ticks_from_frame(1, 15.0) == 4);   // 1/15 * 60 = 4
        CHECK(ticks_from_frame(15, 15.0) == 60); // 1 second = 60 ticks
    }

    SUBCASE("24 fps (film)")
    {
        CHECK(ticks_from_frame(0, 24.0) == 0);
        CHECK(ticks_from_frame(1, 24.0) == 3);   // 1/24 * 60 = 2.5, rounded to 3
        CHECK(ticks_from_frame(24, 24.0) == 60); // 1 second = 60 ticks
    }

    SUBCASE("25 fps (PAL)")
    {
        CHECK(ticks_from_frame(0, 25.0) == 0);
        CHECK(ticks_from_frame(1, 25.0) == 2);   // 1/25 * 60 = 2.4, rounded to 2
        CHECK(ticks_from_frame(25, 25.0) == 60); // 1 second = 60 ticks
    }
}

TEST_CASE("ticks_from_frame - fractional fps")
{
    SUBCASE("29.97 fps (NTSC)")
    {
        CHECK(ticks_from_frame(0, 29.97) == 0);
        auto ticks1 = ticks_from_frame(1, 29.97);
        CHECK(ticks1 == 2); // ~2.002, rounds to 2

        auto ticks30 = ticks_from_frame(30, 29.97);
        // 30/29.97 * 60 ≈ 60.06, rounds to 60
        CHECK(ticks30 == 60);
    }

    SUBCASE("23.976 fps (film on NTSC)")
    {
        CHECK(ticks_from_frame(0, 23.976) == 0);
        auto ticks1 = ticks_from_frame(1, 23.976);
        CHECK(ticks1 == 3); // ~2.502, rounds to 3
    }
}

TEST_CASE("ticks_from_frame - boundary cases")
{
    SUBCASE("frame 0 always gives 0 ticks")
    {
        CHECK(ticks_from_frame(0, 1.0) == 0);
        CHECK(ticks_from_frame(0, 30.0) == 0);
        CHECK(ticks_from_frame(0, 60.0) == 0);
        CHECK(ticks_from_frame(0, 120.0) == 0);
    }

    SUBCASE("very slow fps")
    {
        CHECK(ticks_from_frame(1, 1.0) == 60);  // 1 fps: 1 frame = 1 second = 60 ticks
        CHECK(ticks_from_frame(2, 1.0) == 120); // 2 frames = 2 seconds = 120 ticks
    }

    SUBCASE("very fast fps")
    {
        CHECK(ticks_from_frame(120, 120.0) == 60);  // 120 fps: 120 frames = 1 second
        CHECK(ticks_from_frame(240, 120.0) == 120); // 240 frames = 2 seconds
    }
}

TEST_CASE("ticks_from_frame - rounding behavior")
{
    // Test that rounding works correctly (+ 0.5 for rounding)
    // At 33 fps, 1 frame = 1/33 * 60 = 1.818... ticks
    auto ticks1 = ticks_from_frame(1, 33.0);
    CHECK(ticks1 == 2); // 1.818 + 0.5 = 2.318, truncates to 2

    // At 40 fps, 1 frame = 1/40 * 60 = 1.5 ticks
    auto ticks2 = ticks_from_frame(1, 40.0);
    CHECK(ticks2 == 2); // 1.5 + 0.5 = 2.0
}

TEST_CASE("equals - identical timestamps")
{
    CHECK(equals(0.0, 0.0));
    CHECK(equals(1.0, 1.0));
    CHECK(equals(100.5, 100.5));
    CHECK(equals(-5.0, -5.0));
}

TEST_CASE("equals - within tolerance")
{
    // Tolerance is 1/22050 ≈ 0.0000453 seconds
    double tolerance = 1.0 / 22050.0;

    SUBCASE("just within tolerance")
    {
        CHECK(equals(1.0, 1.0 + tolerance * 0.5));
        CHECK(equals(1.0, 1.0 - tolerance * 0.5));
        CHECK(equals(0.0, tolerance * 0.9));
        CHECK(equals(0.0, -tolerance * 0.9));
    }

    SUBCASE("at exact tolerance boundary")
    {
        // At exactly the tolerance, should still be within (< not <=)
        // Actually the function uses < so exactly at tolerance is NOT equal
        CHECK_FALSE(equals(1.0, 1.0 + tolerance));
        CHECK_FALSE(equals(1.0, 1.0 - tolerance));
    }
}

TEST_CASE("equals - outside tolerance")
{
    double tolerance = 1.0 / 22050.0;

    CHECK_FALSE(equals(1.0, 1.0 + tolerance * 2.0));
    CHECK_FALSE(equals(1.0, 1.0 - tolerance * 2.0));
    CHECK_FALSE(equals(0.0, 0.001));
    CHECK_FALSE(equals(1.0, 2.0));
    CHECK_FALSE(equals(0.0, 1.0));
}

TEST_CASE("equals - negative timestamps")
{
    double tolerance = 1.0 / 22050.0;

    CHECK(equals(-1.0, -1.0));
    CHECK(equals(-1.0, -1.0 + tolerance * 0.5));
    CHECK(equals(-1.0, -1.0 - tolerance * 0.5));
    CHECK_FALSE(equals(-1.0, -2.0));
}

TEST_CASE("equals - zero and near-zero")
{
    double tolerance = 1.0 / 22050.0;

    CHECK(equals(0.0, 0.0));
    CHECK(equals(0.0, tolerance * 0.5));
    CHECK(equals(0.0, -tolerance * 0.5));
    CHECK_FALSE(equals(0.0, tolerance * 2.0));
}

TEST_CASE("equals - large timestamps")
{
    double tolerance = 1.0 / 22050.0;

    CHECK(equals(1000.0, 1000.0));
    CHECK(equals(1000.0, 1000.0 + tolerance * 0.5));
    CHECK(equals(1000.0, 1000.0 - tolerance * 0.5));
    CHECK_FALSE(equals(1000.0, 1001.0));
}

TEST_CASE("equals - symmetric")
{
    // equals should be symmetric: equals(a, b) == equals(b, a)
    double tolerance = 1.0 / 22050.0;

    CHECK(equals(1.0, 1.0 + tolerance * 0.5) == equals(1.0 + tolerance * 0.5, 1.0));
    CHECK(equals(0.0, 0.001) == equals(0.001, 0.0));
    CHECK(equals(5.0, 5.5) == equals(5.5, 5.0));
}

} // namespace macflim
