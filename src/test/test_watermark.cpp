#include "../doctest.h"

#include "../grayscale.hpp"
#include "../watermark.hpp"

using namespace macflim;

TEST_CASE("watermark: basic single character")
{
    grayscale img(512, 342);
    fill(img, 0.5);

    watermark(img, "A");

    // Verify character was drawn at position (8, 3)
    // Character should have a black line on top (y=3)
    CHECK(img.at(8, 3) == 0);
    CHECK(img.at(15, 3) == 0);

    // Verify some pixels in the character body exist (not all 0.5)
    bool has_drawn_pixels = false;
    for (int y = 4; y < 13; y++)
    {
        for (int x = 8; x < 16; x++)
        {
            if (img.at(x, y) != 0.5)
            {
                has_drawn_pixels = true;
                break;
            }
        }
    }
    CHECK(has_drawn_pixels);
}

TEST_CASE("watermark: multiple characters on same line")
{
    grayscale img(512, 342);
    fill(img, 0.5);

    watermark(img, "ABC");

    // Verify first character at (8, 3)
    CHECK(img.at(8, 3) == 0);

    // Verify second character at (8 + 8, 3) = (16, 3)
    CHECK(img.at(16, 3) == 0);

    // Verify third character at (8 + 16, 3) = (24, 3)
    CHECK(img.at(24, 3) == 0);
}

TEST_CASE("watermark: newline moves to next line")
{
    grayscale img(512, 342);
    fill(img, 0.5);

    watermark(img, "A\nB");

    // First character at y=3
    CHECK(img.at(8, 3) == 0);

    // Second character should be on next line at y=3+8=11 (one 8-pixel row down)
    // Actually y should be 0*8+3 for first line, then 1*8+3 for second line
    CHECK(img.at(8, 11) == 0);
}

TEST_CASE("watermark: wraps at 62 characters")
{
    grayscale img(512, 342);
    fill(img, 0.5);

    // Create a string with exactly 63 characters (should wrap after 62)
    std::string long_text(63, 'X');
    watermark(img, long_text);

    // 62nd character should be on first line at x = 8 + 61*8
    CHECK(img.at(8 + 61 * 8, 3) == 0);

    // 63rd character should wrap to second line at x = 8, y = 11
    CHECK(img.at(8, 11) == 0);
}

TEST_CASE("watermark: control characters below 32 are skipped except newline")
{
    grayscale img(512, 342);
    fill(img, 0.5);

    // Tab (9) and other control chars should be skipped
    watermark(img, "\tA");

    // 'A' should still be at the first position since tab is skipped
    CHECK(img.at(8, 3) == 0);

    // Verify no drawing happened before position 8
    bool has_pixels_before = false;
    for (int x = 0; x < 8; x++)
    {
        if (img.at(x, 3) != 0.5)
        {
            has_pixels_before = true;
            break;
        }
    }
    CHECK_FALSE(has_pixels_before);
}

TEST_CASE("watermark: space character is printable")
{
    grayscale img(512, 342);
    fill(img, 0.5);

    watermark(img, " B");

    // Space (32) is printable, so it should render
    // Space at position (8, 3)
    CHECK(img.at(8, 3) == 0);

    // 'B' at position (16, 3)
    CHECK(img.at(16, 3) == 0);
}

TEST_CASE("watermark: empty string does nothing")
{
    grayscale img(512, 342);
    fill(img, 0.5);

    watermark(img, "");

    // Check nothing was drawn
    bool all_unchanged = true;
    for (size_t y = 0; y < img.H(); y++)
    {
        for (size_t x = 0; x < img.W(); x++)
        {
            if (img.at(x, y) != 0.5)
            {
                all_unchanged = false;
                break;
            }
        }
        if (!all_unchanged)
            break;
    }
    CHECK(all_unchanged);
}

TEST_CASE("burn_subtitle: basic centering")
{
    grayscale img(512, 342);
    fill(img, 0.5);

    burn_subtitle(img, "TEST");

    // Subtitle should be at y = H - 9 - 3 = 342 - 12 = 330
    size_t y = 330;

    // char_width = 512 / 8 = 64
    // text is " TEST " (padded with spaces) = 6 chars
    // centered position = (64 - 6) / 2 = 29
    // x position in pixels = 29 * 8 = 232

    // Check that character was drawn around the expected position
    // The black line should be visible
    bool has_drawn = false;
    for (size_t x = 200; x < 300; x++)
    {
        if (img.at(x, y) == 0)
        {
            has_drawn = true;
            break;
        }
    }
    CHECK(has_drawn);
}

TEST_CASE("burn_subtitle: truncates long text")
{
    grayscale img(512, 342);
    fill(img, 0.5);

    // char_width = 512 / 8 = 64
    // Create a string longer than 64 characters
    std::string long_text(100, 'X');

    burn_subtitle(img, long_text);

    // Should not crash and should truncate to char_width
    // Verify subtitle was rendered at bottom
    size_t y = img.H() - 9 - 3;
    bool has_drawn = false;
    for (size_t x = 0; x < img.W(); x++)
    {
        if (img.at(x, y) == 0)
        {
            has_drawn = true;
            break;
        }
    }
    CHECK(has_drawn);
}

TEST_CASE("burn_subtitle: empty subtitle")
{
    grayscale img(512, 342);
    fill(img, 0.5);

    burn_subtitle(img, "");

    // Should render just the padding spaces
    // " " becomes "  " with padding, which gets centered
    size_t y = img.H() - 9 - 3;

    // Should have drawn the spaces (they have black top line)
    bool has_black_line = false;
    for (size_t x = 0; x < img.W(); x++)
    {
        if (img.at(x, y) == 0)
        {
            has_black_line = true;
            break;
        }
    }
    CHECK(has_black_line);
}

TEST_CASE("burn_subtitle: small image dimensions")
{
    grayscale img(64, 50);
    fill(img, 0.5);

    burn_subtitle(img, "Hi");

    // char_width = 64 / 8 = 8
    // text " Hi " = 4 chars
    // centered at (8 - 4) / 2 = 2
    // y = 50 - 9 - 3 = 38

    size_t y = 38;
    bool has_drawn = false;
    for (size_t x = 0; x < img.W(); x++)
    {
        if (img.at(x, y) == 0)
        {
            has_drawn = true;
            break;
        }
    }
    CHECK(has_drawn);
}

TEST_CASE("burn_subtitle: characters are 8x9 pixels")
{
    grayscale img(512, 342);
    fill(img, 0.5);

    burn_subtitle(img, "I");

    // Verify the character occupies 9 rows (1 black line + 8 font rows)
    size_t y_start = img.H() - 9 - 3;
    size_t y_end = y_start + 9;

    // Check that pixels were modified in this range
    int modified_rows = 0;
    for (size_t y = y_start; y < y_end; y++)
    {
        for (size_t x = 0; x < img.W(); x++)
        {
            if (img.at(x, y) != 0.5)
            {
                modified_rows++;
                break;
            }
        }
    }

    // Should have modified multiple rows (at least the black line + some font data)
    CHECK(modified_rows >= 2);
}
