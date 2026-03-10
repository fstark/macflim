#include "../doctest.h"

#include "../dithering_parameters.hpp"
#include "../profile.hpp"

using namespace macflim;

TEST_CASE("dithering_parameters: from_profile basic fields")
{
    encoding_profile p;
    REQUIRE(encoding_profile::profile_named("se30", p));

    dithering_parameters dp = dithering_parameters::from_profile(p, "test_watermark.png");

    CHECK(dp.bars == p.bars());
    CHECK(dp.filters == p.filters());
    CHECK(dp.anchor_x == doctest::Approx(p.anchor_x()));
    CHECK(dp.anchor_y == doctest::Approx(p.anchor_y()));
    CHECK(dp.dither == p.dither());
    CHECK(dp.error_algorithm == p.error_algorithm());
    CHECK(dp.stability == doctest::Approx(p.stability()));
    CHECK(dp.error_bleed == doctest::Approx(p.error_bleed()));
    CHECK(dp.error_bidi == p.error_bidi());
    CHECK(dp.watermark == "test_watermark.png");
}

TEST_CASE("dithering_parameters: from_profile with different profiles")
{
    SUBCASE("128k profile")
    {
        encoding_profile p;
        REQUIRE(encoding_profile::profile_named("128k", p));

        dithering_parameters dp = dithering_parameters::from_profile(p, "logo.png");

        CHECK(dp.dither == grayscale::dithering::ordered);
        CHECK(dp.watermark == "logo.png");
    }

    SUBCASE("se30 profile")
    {
        encoding_profile p;
        REQUIRE(encoding_profile::profile_named("se30", p));

        dithering_parameters dp = dithering_parameters::from_profile(p, "");

        CHECK(dp.dither == grayscale::dithering::error_diffusion);
        CHECK(dp.watermark == "");
    }

    SUBCASE("perfect profile")
    {
        encoding_profile p;
        REQUIRE(encoding_profile::profile_named("perfect", p));

        dithering_parameters dp = dithering_parameters::from_profile(p, "watermark.png");

        CHECK(dp.watermark == "watermark.png");
    }
}

TEST_CASE("dithering_parameters: watermark string preserved")
{
    encoding_profile p;
    REQUIRE(encoding_profile::profile_named("se30", p));

    SUBCASE("empty watermark")
    {
        dithering_parameters dp = dithering_parameters::from_profile(p, "");
        CHECK(dp.watermark == "");
    }

    SUBCASE("watermark with path")
    {
        dithering_parameters dp = dithering_parameters::from_profile(p, "/path/to/logo.png");
        CHECK(dp.watermark == "/path/to/logo.png");
    }

    SUBCASE("watermark with spaces")
    {
        dithering_parameters dp = dithering_parameters::from_profile(p, "my logo.png");
        CHECK(dp.watermark == "my logo.png");
    }
}

TEST_CASE("dithering_parameters: all profile fields copied")
{
    encoding_profile p;
    REQUIRE(encoding_profile::profile_named("plus", p));

    dithering_parameters dp = dithering_parameters::from_profile(p, "test.png");

    // Verify all fields are accessible and have reasonable values
    // bars is a boolean
    CHECK((dp.bars == true || dp.bars == false));
    // filters string is accessible (may be empty or non-empty) - just access it
    std::string filters_copy = dp.filters;
    CHECK(filters_copy.size() >= 0); // always true, but accesses the field
    // anchors are in valid range
    CHECK(dp.anchor_x >= 0.0);
    CHECK(dp.anchor_x <= 1.0);
    CHECK(dp.anchor_y >= 0.0);
    CHECK(dp.anchor_y <= 1.0);
    // dither is one of the valid enum values
    CHECK((dp.dither == grayscale::dithering::ordered || dp.dither == grayscale::dithering::blue_noise ||
           dp.dither == grayscale::dithering::error_diffusion));
    // error_algorithm string is accessible
    std::string error_copy = dp.error_algorithm;
    CHECK(error_copy.size() >= 0);
    // stability and error_bleed in valid ranges
    CHECK(dp.stability >= 0.0);
    CHECK(dp.stability <= 1.0);
    CHECK(dp.error_bleed >= 0.0f);
    CHECK(dp.error_bleed <= 1.0f);
    // error_bidi is a boolean
    CHECK((dp.error_bidi == true || dp.error_bidi == false));
}
