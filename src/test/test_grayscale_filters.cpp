#include "../doctest.h"

#include "../errors.hpp"
#include "../grayscale.hpp"
#include <cmath>

namespace macflim
{

TEST_CASE("sharpen - edge enhancement (via filter)")
{
    grayscale src(5, 5);
    fill(src, 0.5f);

    // Create an edge: left half black, right half white
    for (size_t y = 0; y < 5; y++)
    {
        src.at(0, y) = 0.0f;
        src.at(1, y) = 0.0f;
        src.at(3, y) = 1.0f;
        src.at(4, y) = 1.0f;
    }

    grayscale result = filter(src, "s"); // 's' = sharpen

    CHECK(result.W() == 5);
    CHECK(result.H() == 5);

    // Sharpening should enhance edges (not testing exact values, just structure)
    // Border pixels remain unchanged in sharpen (only interior 1 to W-2)
}

TEST_CASE("blur3 - 3x3 blur (via filter)")
{
    grayscale src(5, 5);
    fill(src, 0.0f);

    // Single white pixel in center
    src.at(2, 2) = 1.0f;

    grayscale result = filter(src, "b"); // 'b' or 'b3' = blur3

    CHECK(result.W() == 5);
    CHECK(result.H() == 5);

    // Center should still be brightest but not 1.0 (blurred)
    // Neighbors should have received some light
    CHECK(result.at(2, 2) > 0.0f);
    CHECK(result.at(2, 2) < 1.0f); // Should be less than original due to blur

    // Adjacent pixels should be non-zero
    CHECK(result.at(1, 2) > 0.0f);
    CHECK(result.at(3, 2) > 0.0f);
    CHECK(result.at(2, 1) > 0.0f);
    CHECK(result.at(2, 3) > 0.0f);
}

TEST_CASE("blur5 - 5x5 blur (via filter)")
{
    grayscale src(10, 10);
    fill(src, 0.0f);

    // Single white pixel in center
    src.at(5, 5) = 1.0f;

    grayscale result = filter(src, "b5"); // 'b5' = blur5

    CHECK(result.W() == 10);
    CHECK(result.H() == 10);

    // Center should still be brightest but blurred
    CHECK(result.at(5, 5) > 0.0f);

    // More distant neighbors should have light too (5x5 kernel)
    CHECK(result.at(3, 5) > 0.0f);
    CHECK(result.at(7, 5) > 0.0f);
}

TEST_CASE("gamma - gamma correction (via filter)")
{
    SUBCASE("gamma > 1 darkens midtones")
    {
        grayscale src(5, 5);
        src.at(0, 0) = 0.0f; // black stays black
        src.at(1, 1) = 0.5f; // midtone
        src.at(2, 2) = 1.0f; // white stays white

        grayscale result = filter(src, "g2.0"); // 'g2.0' = gamma 2.0

        CHECK(result.at(0, 0) == doctest::Approx(0.0f)); // black unchanged
        CHECK(result.at(1, 1) < 0.5f);                   // midtone darkened
        CHECK(result.at(2, 2) == doctest::Approx(1.0f)); // white unchanged
    }

    SUBCASE("gamma < 1 lightens midtones")
    {
        grayscale src(5, 5);
        src.at(0, 0) = 0.0f;
        src.at(1, 1) = 0.5f;
        src.at(2, 2) = 1.0f;

        grayscale result = filter(src, "g0.5"); // 'g0.5' = gamma 0.5

        CHECK(result.at(0, 0) == doctest::Approx(0.0f));
        CHECK(result.at(1, 1) > 0.5f); // midtone lightened
        CHECK(result.at(2, 2) == doctest::Approx(1.0f));
    }

    SUBCASE("gamma = 1 is identity")
    {
        grayscale src(5, 5);
        src.at(1, 1) = 0.5f;

        grayscale result = filter(src, "g1.0"); // 'g1.0' = gamma 1.0

        CHECK(result.at(1, 1) == doctest::Approx(0.5f));
    }
}

TEST_CASE("black - remove darkest pixels (via filter)")
{
    grayscale src(5, 5);

    // Create gradient: 0.0, 0.1, 0.2, ..., 1.0
    for (size_t i = 0; i < 5; i++)
        for (size_t j = 0; j < 5; j++)
            src.at(i, j) = (i * 5 + j) / 24.0f;

    grayscale result = filter(src, "k0.1"); // 'k0.1' = black threshold 0.1

    CHECK(result.W() == 5);
    CHECK(result.H() == 5);

    // Darkest pixels should be remapped to black
    // Brightest should remain near 1.0
    // Just verify structure
    CHECK(result.at(0, 0) <= src.at(0, 0));
    CHECK(result.at(4, 4) >= 0.8f);
}

TEST_CASE("white - remove brightest pixels (via filter)")
{
    grayscale src(5, 5);

    // Create gradient
    for (size_t i = 0; i < 5; i++)
        for (size_t j = 0; j < 5; j++)
            src.at(i, j) = (i * 5 + j) / 24.0f;

    grayscale result = filter(src, "w0.1"); // 'w0.1' = white threshold

    CHECK(result.W() == 5);
    CHECK(result.H() == 5);

    // Brightest pixels should be remapped
    // Verify structure preserved
    CHECK(result.at(0, 0) <= 0.2f);
}

TEST_CASE("quantize - reduce color levels (via filter)")
{
    grayscale src(5, 5);

    // Create smooth gradient
    for (size_t i = 0; i < 5; i++)
        for (size_t j = 0; j < 5; j++)
            src.at(i, j) = (i * 5 + j) / 24.0f;

    grayscale result = filter(src, "q3"); // 'q3' = quantize to 3 levels

    CHECK(result.W() == 5);
    CHECK(result.H() == 5);

    // All pixels should be snapped to one of the quantized levels
    for (size_t i = 0; i < 5; i++)
    {
        for (size_t j = 0; j < 5; j++)
        {
            float val = result.at(i, j);
            // Should be close to 0.0, 0.5, or 1.0 for 3 levels
            bool is_quantized =
                (std::abs(val - 0.0f) < 0.01f) || (std::abs(val - 0.5f) < 0.01f) || (std::abs(val - 1.0f) < 0.01f);
            CHECK(is_quantized);
        }
    }
}

TEST_CASE("filter - single filter application")
{
    grayscale src(10, 10);
    fill(src, 0.5f);

    SUBCASE("blur filter")
    {
        grayscale result = filter(src, "b");
        CHECK(result.W() == 10);
        CHECK(result.H() == 10);
    }

    SUBCASE("sharpen filter")
    {
        grayscale result = filter(src, "s");
        CHECK(result.W() == 10);
        CHECK(result.H() == 10);
    }

    SUBCASE("flip filter")
    {
        src.at(0, 0) = 0.0f;
        src.at(9, 0) = 1.0f;

        grayscale result = filter(src, "f");

        CHECK(result.at(0, 0) == doctest::Approx(1.0f));
        CHECK(result.at(9, 0) == doctest::Approx(0.0f));
    }

    SUBCASE("invert filter")
    {
        src.at(0, 0) = 0.0f;
        src.at(1, 1) = 1.0f;

        grayscale result = filter(src, "i");

        CHECK(result.at(0, 0) == doctest::Approx(1.0f));
        CHECK(result.at(1, 1) == doctest::Approx(0.0f));
    }

    SUBCASE("gamma filter with argument")
    {
        src.at(5, 5) = 0.5f;

        grayscale result = filter(src, "g2.0");

        CHECK(result.at(5, 5) < 0.5f); // Gamma 2.0 darkens midtones
    }
}

TEST_CASE("filter - multiple filters chained")
{
    grayscale src(10, 10);
    fill(src, 0.5f);
    src.at(0, 0) = 0.0f;
    src.at(9, 0) = 1.0f;

    SUBCASE("flip then invert")
    {
        grayscale result = filter(src, "fi");

        // First flipped: (0,0)=1.0, (9,0)=0.0
        // Then inverted: (0,0)=0.0, (9,0)=1.0
        CHECK(result.at(0, 0) == doctest::Approx(0.0f));
        CHECK(result.at(9, 0) == doctest::Approx(1.0f));
    }

    SUBCASE("blur with argument")
    {
        grayscale result = filter(src, "b3");
        CHECK(result.W() == 10);
        CHECK(result.H() == 10);
    }

    SUBCASE("quantize with argument")
    {
        grayscale result = filter(src, "q5");
        CHECK(result.W() == 10);
        CHECK(result.H() == 10);
    }
}

TEST_CASE("debug_filter - debug border (via filter '@')")
{
    grayscale src(10, 10);
    fill(src, 0.5f);

    grayscale result = filter(src, "@");

    // Debug filter adds a white-black border pattern
    // Top edge: white line at y=0, black at y=1
    CHECK(result.at(2, 0) == doctest::Approx(1.0f));
    CHECK(result.at(2, 1) == doctest::Approx(0.0f));

    // Bottom edge: white at y=H-1, black at y=H-2
    CHECK(result.at(2, 9) == doctest::Approx(1.0f));
    CHECK(result.at(2, 8) == doctest::Approx(0.0f));

    // Left edge: white at x=0, black at x=1
    CHECK(result.at(0, 5) == doctest::Approx(1.0f));
    CHECK(result.at(1, 5) == doctest::Approx(0.0f));

    // Right edge: white at x=W-1, black at x=W-2
    CHECK(result.at(9, 5) == doctest::Approx(1.0f));
    CHECK(result.at(8, 5) == doctest::Approx(0.0f));

    // Center should be unchanged
    CHECK(result.at(5, 5) == doctest::Approx(0.5f));
}

TEST_CASE("black - edge case with high cutoff value")
{
    grayscale src(5, 5);
    fill(src, 0.1f); // Very dark gray

    // With a high black cutoff (20%), pixels at 0.1 should:
    // v = (0.1 - 0.2) / (1 - 0.2) = -0.1 / 0.8 = negative
    // Should be clamped to 0
    grayscale result = filter(src, "k20");

    // Result should be black (clamped to 0)
    CHECK(result.at(2, 2) == doctest::Approx(0.0f));
}

TEST_CASE("white - edge case with high cutoff value")
{
    grayscale src(5, 5);
    fill(src, 0.9f); // Very bright gray

    // With a high white boost (50%), pixels at 0.9 should:
    // v = 0.9 * (1 + 0.5) = 0.9 * 1.5 = 1.35
    // Should be clamped to 1.0
    grayscale result = filter(src, "w50");

    // Result should be white (clamped to 1.0)
    CHECK(result.at(2, 2) == doctest::Approx(1.0f));
}

TEST_CASE("zoom_out - edge case with out-of-bounds access")
{
    grayscale src(32, 32);
    fill(src, 0.8f);

    // With small zoom (8 pixels), the function should work correctly
    grayscale result = filter(src, "z8");

    // Should preserve dimensions
    CHECK(result.W() == 32);
    CHECK(result.H() == 32);

    // At least verify it doesn't crash and produces valid output
    for (size_t y = 0; y < result.H(); y++)
    {
        for (size_t x = 0; x < result.W(); x++)
        {
            float v = result.at(x, y);
            CHECK(v >= 0.0f);
            CHECK(v <= 1.0f);
        }
    }

    // Test with larger zoom parameter to trigger bounds checking
    grayscale result2 = filter(src, "z16");
    CHECK(result2.W() == 32);
    CHECK(result2.H() == 32);
}

TEST_CASE("zoom_out - returns black image when bx*2 >= W")
{
    grayscale src(32, 32);
    fill(src, 0.8f);

    // zoom_out with bx=16 means bx*2 = 32 = W, should return black
    grayscale result = filter(src, "z16");

    // All pixels should be black
    for (size_t y = 0; y < result.H(); y++)
    {
        for (size_t x = 0; x < result.W(); x++)
        {
            CHECK(result.at(x, y) == doctest::Approx(0.0f));
        }
    }

    // Test with even larger zoom - should also be black
    grayscale result2 = filter(src, "z50");
    for (size_t y = 0; y < result2.H(); y++)
    {
        for (size_t x = 0; x < result2.W(); x++)
        {
            CHECK(result2.at(x, y) == doctest::Approx(0.0f));
        }
    }
}

TEST_CASE("blue_noise_dither - basic pattern")
{
    grayscale src(10, 10);
    fill(src, 0.5f); // 50% gray

    grayscale dest(10, 10);
    grayscale previous(10, 10);
    fill(previous, 0.0f);

    blue_noise_dither(dest, src, previous);

    // Should create a binary (0 or 1) image
    for (size_t y = 0; y < dest.H(); y++)
    {
        for (size_t x = 0; x < dest.W(); x++)
        {
            float val = dest.at(x, y);
            CHECK((val == 0.0f || val == 1.0f));
        }
    }

    // For 50% gray, roughly half should be white, half black
    int white_count = 0;
    int total = dest.W() * dest.H();
    for (size_t y = 0; y < dest.H(); y++)
    {
        for (size_t x = 0; x < dest.W(); x++)
        {
            if (dest.at(x, y) == 1.0f)
                white_count++;
        }
    }
    // Allow some tolerance (30%-70% white for 50% input)
    CHECK(white_count > total * 0.3);
    CHECK(white_count < total * 0.7);
}

TEST_CASE("blue_noise_dither - black and white inputs")
{
    grayscale previous(5, 5);
    fill(previous, 0.0f);

    SUBCASE("fully black input")
    {
        grayscale src(5, 5);
        fill(src, 0.0f);
        grayscale dest(5, 5);

        blue_noise_dither(dest, src, previous);

        // All pixels should be black
        for (size_t y = 0; y < dest.H(); y++)
        {
            for (size_t x = 0; x < dest.W(); x++)
            {
                CHECK(dest.at(x, y) == 0.0f);
            }
        }
    }

    SUBCASE("fully white input")
    {
        grayscale src(5, 5);
        fill(src, 1.0f);
        grayscale dest(5, 5);

        blue_noise_dither(dest, src, previous);

        // All pixels should be white
        for (size_t y = 0; y < dest.H(); y++)
        {
            for (size_t x = 0; x < dest.W(); x++)
            {
                CHECK(dest.at(x, y) == 1.0f);
            }
        }
    }
}

TEST_CASE("filter - error handling for invalid blur argument")
{
    grayscale src(5, 5);
    fill(src, 0.5f);

    // Blur with invalid size (only 3 and 5 are allowed)
    bool caught = false;
    try
    {
        [[maybe_unused]] auto result = filter(src, "b7");
    }
    catch (const config_error &)
    {
        caught = true;
    }
    CHECK(caught);
}

TEST_CASE("filter - error handling for unknown filter")
{
    grayscale src(5, 5);
    fill(src, 0.5f);

    // Use invalid filter character
    bool caught1 = false;
    try
    {
        [[maybe_unused]] auto result = filter(src, "x");
    }
    catch (const config_error &)
    {
        caught1 = true;
    }
    CHECK(caught1);

    bool caught2 = false;
    try
    {
        [[maybe_unused]] auto result = filter(src, "Q");
    }
    catch (const config_error &)
    {
        caught2 = true;
    }
    CHECK(caught2);
}

} // namespace macflim
