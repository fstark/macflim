#include "doctest.h"

#include "ditherer.hpp"

using namespace macflim;

static constexpr size_t W = 512;
static constexpr size_t H = 342;

static grayscale uniform_gray(float value)
{
    grayscale img(W, H);
    fill(img, value);
    return img;
}

static dithering_parameters make_error_dp()
{
    return dithering_parameters{
        false, // bars
        "c",   // filters
        0.5,   // anchor_x
        0.5,   // anchor_y
        grayscale::dithering::error_diffusion,
        "floyd", // error_algorithm
        0.3,     // stability
        0.99f,   // error_bleed
        false,   // error_bidi
        ""       // watermark
    };
}

static dithering_parameters make_ordered_dp()
{
    return dithering_parameters{
        false, // bars
        "c",   // filters
        0.5,   // anchor_x
        0.5,   // anchor_y
        grayscale::dithering::ordered,
        "floyd", // error_algorithm
        0.3,     // stability
        0.99f,   // error_bleed
        false,   // error_bidi
        ""       // watermark
    };
}

// --- output dimensions ---

TEST_CASE("ditherer: output dimensions match input")
{
    auto img = uniform_gray(0.5);
    auto dp = make_error_dp();
    ditherer d(img, dp);

    d.dither(img);
    auto result = d.current();

    CHECK(result.W() == W);
    CHECK(result.H() == H);
}

// --- output is binary ---

TEST_CASE("ditherer: output is binary")
{
    auto img = uniform_gray(0.5);
    auto dp = make_error_dp();
    ditherer d(img, dp);

    d.dither(img);
    auto result = d.current();

    for (size_t y = 0; y < H; y++)
        for (size_t x = 0; x < W; x++)
        {
            float v = result.at(x, y);
            CHECK((v == 0.0f || v == 1.0f));
        }
}

// --- temporal stability ---

TEST_CASE("ditherer: same input twice produces identical output")
{
    auto img = uniform_gray(0.5);
    auto dp = make_error_dp();

    ditherer d1(img, dp);
    d1.dither(img);
    auto r1 = d1.current();

    ditherer d2(img, dp);
    d2.dither(img);
    auto r2 = d2.current();

    // Both should produce identical results from same initial state
    for (size_t y = 0; y < H; y++)
        for (size_t x = 0; x < W; x++)
            CHECK(r1.at(x, y) == r2.at(x, y));
}

// --- ordered dithering: 50% gray → ~50% black pixels ---

TEST_CASE("ditherer: ordered dithering 50% gray")
{
    auto img = uniform_gray(0.5);
    auto dp = make_ordered_dp();
    ditherer d(img, dp);

    d.dither(img);
    auto result = d.current();

    size_t black_count = 0;
    for (size_t y = 0; y < H; y++)
        for (size_t x = 0; x < W; x++)
            if (result.at(x, y) == 0.0f)
                black_count++;

    double black_ratio = static_cast<double>(black_count) / (W * H);
    // Should be approximately 50% black (±10%)
    CHECK(black_ratio > 0.40);
    CHECK(black_ratio < 0.60);
}

// --- gradient coverage ---

TEST_CASE("ditherer: gradient produces increasing density")
{
    // Create a vertical gradient: black at top, white at bottom
    grayscale img(W, H);
    for (size_t y = 0; y < H; y++)
        for (size_t x = 0; x < W; x++)
            img.at(x, y) = static_cast<float>(y) / static_cast<float>(H - 1);

    auto dp = make_ordered_dp();
    ditherer d(img, dp);

    d.dither(img);
    auto result = d.current();

    // Count white pixels (value=1) in top quarter vs bottom quarter
    size_t top_white = 0;
    size_t bottom_white = 0;
    size_t quarter = H / 4;

    for (size_t y = 0; y < quarter; y++)
        for (size_t x = 0; x < W; x++)
            if (result.at(x, y) == 1.0f)
                top_white++;

    for (size_t y = H - quarter; y < H; y++)
        for (size_t x = 0; x < W; x++)
            if (result.at(x, y) == 1.0f)
                bottom_white++;

    // Bottom quarter (brighter) should have more white pixels
    CHECK(bottom_white > top_white);
}
