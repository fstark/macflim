#include "../doctest.h"

#include "../grayscale.hpp"
#include <cmath>

namespace macflim
{

TEST_CASE("grayscale - construction and basic access")
{
    SUBCASE("construct and check dimensions")
    {
        grayscale img(100, 50);
        CHECK(img.W() == 100);
        CHECK(img.H() == 50);
    }

    SUBCASE("at() read/write access")
    {
        grayscale img(10, 10);
        img.at(5, 3) = 0.75f;
        CHECK(img.at(5, 3) == doctest::Approx(0.75f));

        img.at(0, 0) = 0.0f;
        img.at(9, 9) = 1.0f;
        CHECK(img.at(0, 0) == doctest::Approx(0.0f));
        CHECK(img.at(9, 9) == doctest::Approx(1.0f));
    }
}

TEST_CASE("grayscale::set_luma - from uint8_t buffer")
{
    SUBCASE("black buffer")
    {
        grayscale img(4, 4);
        std::vector<uint8_t> buffer(16, 0); // all black

        img.set_luma(buffer.data());

        for (size_t y = 0; y < 4; y++)
            for (size_t x = 0; x < 4; x++)
                CHECK(img.at(x, y) == doctest::Approx(0.0f));
    }

    SUBCASE("white buffer")
    {
        grayscale img(4, 4);
        std::vector<uint8_t> buffer(16, 255); // all white

        img.set_luma(buffer.data());

        for (size_t y = 0; y < 4; y++)
            for (size_t x = 0; x < 4; x++)
                CHECK(img.at(x, y) == doctest::Approx(1.0f));
    }

    SUBCASE("mixed values")
    {
        grayscale img(2, 2);
        std::vector<uint8_t> buffer = {0, 127, 128, 255};

        img.set_luma(buffer.data());

        CHECK(img.at(0, 0) == doctest::Approx(0.0f / 255.0f));
        CHECK(img.at(1, 0) == doctest::Approx(127.0f / 255.0f).epsilon(0.01));
        CHECK(img.at(0, 1) == doctest::Approx(128.0f / 255.0f).epsilon(0.01));
        CHECK(img.at(1, 1) == doctest::Approx(1.0f));
    }
}

TEST_CASE("fill - constant color")
{
    SUBCASE("fill with default (0.5 gray)")
    {
        grayscale img(10, 10);
        fill(img);

        for (size_t y = 0; y < 10; y++)
            for (size_t x = 0; x < 10; x++)
                CHECK(img.at(x, y) == doctest::Approx(0.5f));
    }

    SUBCASE("fill with black")
    {
        grayscale img(10, 10);
        fill(img, 0.0f);

        for (size_t y = 0; y < 10; y++)
            for (size_t x = 0; x < 10; x++)
                CHECK(img.at(x, y) == doctest::Approx(0.0f));
    }

    SUBCASE("fill with white")
    {
        grayscale img(10, 10);
        fill(img, 1.0f);

        for (size_t y = 0; y < 10; y++)
            for (size_t x = 0; x < 10; x++)
                CHECK(img.at(x, y) == doctest::Approx(1.0f));
    }

    SUBCASE("fill with custom value")
    {
        grayscale img(5, 5);
        fill(img, 0.75f);

        for (size_t y = 0; y < 5; y++)
            for (size_t x = 0; x < 5; x++)
                CHECK(img.at(x, y) == doctest::Approx(0.75f));
    }
}

TEST_CASE("copy - resize with black bars / letterboxing")
{
    SUBCASE("same size - identity copy")
    {
        grayscale src(10, 10);
        fill(src, 0.6f);
        src.at(5, 5) = 1.0f;

        grayscale dest(10, 10);
        copy(dest, src);

        CHECK(dest.at(5, 5) == doctest::Approx(1.0f));
        CHECK(dest.at(0, 0) == doctest::Approx(0.6f));
    }

    SUBCASE("downscale with black bars")
    {
        grayscale src(20, 20);
        fill(src, 0.8f);

        grayscale dest(10, 10);
        copy(dest, src, true); // black_bars = true

        // At minimum, function should not crash and preserve dimensions
        CHECK(dest.W() == 10);
        CHECK(dest.H() == 10);
    }
}

TEST_CASE("flip - horizontal mirror (via filter)")
{
    grayscale src(5, 3);
    fill(src, 0.5f);

    // Create a pattern: column 0 is black, column 4 is white
    for (size_t y = 0; y < 3; y++)
    {
        src.at(0, y) = 0.0f;
        src.at(4, y) = 1.0f;
    }

    grayscale result = filter(src, "f"); // 'f' = flip

    // After flip: column 0 should be white, column 4 should be black
    CHECK(result.at(0, 0) == doctest::Approx(1.0f));
    CHECK(result.at(0, 1) == doctest::Approx(1.0f));
    CHECK(result.at(4, 0) == doctest::Approx(0.0f));
    CHECK(result.at(4, 1) == doctest::Approx(0.0f));

    // Middle column unchanged
    CHECK(result.at(2, 0) == doctest::Approx(0.5f));
}

TEST_CASE("invert - color inversion (via filter)")
{
    grayscale src(5, 5);
    src.at(0, 0) = 0.0f; // black → white
    src.at(1, 1) = 1.0f; // white → black
    src.at(2, 2) = 0.5f; // gray → gray
    src.at(3, 3) = 0.25f;

    grayscale result = filter(src, "i"); // 'i' = invert

    CHECK(result.at(0, 0) == doctest::Approx(1.0f));
    CHECK(result.at(1, 1) == doctest::Approx(0.0f));
    CHECK(result.at(2, 2) == doctest::Approx(0.5f));
    CHECK(result.at(3, 3) == doctest::Approx(0.75f));
}

TEST_CASE("zoom_out - zoom out with border (via filter)")
{
    grayscale src(20, 20);
    fill(src, 0.7f);

    // Put a bright pixel in the center
    src.at(10, 10) = 1.0f;

    grayscale result = filter(src, "z"); // 'z' = zoom_out (default 32 pixels)

    // Dimensions unchanged
    CHECK(result.W() == 20);
    CHECK(result.H() == 20);

    // Edges should have border (darker)
    // Center should still have some of the original content
    // Just verify it doesn't crash and returns valid dimensions
}

TEST_CASE("zoom_in - zoom in / crop (via filter)")
{
    grayscale src(20, 20);
    fill(src, 0.5f);

    // Center pixel
    src.at(10, 10) = 1.0f;

    grayscale result = filter(src, "Z"); // 'Z' = zoom_in (default 32 pixels)

    // Dimensions unchanged
    CHECK(result.W() == 20);
    CHECK(result.H() == 20);

    // Should have zoomed into center region
    // Just verify it works without crashing (validates bounds checking bug fix)
}

TEST_CASE("round_corners - corner rounding")
{
    grayscale src(20, 20);
    fill(src, 1.0f); // all white

    grayscale result = round_corners(src);

    CHECK(result.W() == 20);
    CHECK(result.H() == 20);

    // Corners should be darkened
    // Corner pixels should be less than center
    CHECK(result.at(0, 0) < 1.0f);
    CHECK(result.at(19, 0) < 1.0f);
    CHECK(result.at(0, 19) < 1.0f);
    CHECK(result.at(19, 19) < 1.0f);

    // Center should be unchanged
    CHECK(result.at(10, 10) == doctest::Approx(1.0f));
}

} // namespace macflim
