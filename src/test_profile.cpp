#include "doctest.h"

#include "profile.hpp"

using namespace macflim;

// --- known profile: se30 ---

TEST_CASE("profile: se30")
{
    encoding_profile p;
    REQUIRE(encoding_profile::profile_named("se30", p));

    CHECK(p.width() == 512);
    CHECK(p.height() == 342);
    CHECK(p.byterate() == 6000);
    CHECK(p.group() == true);
    CHECK(p.dither() == grayscale::dithering::error_diffusion);
    CHECK(p.fps_ratio() == 1);
    CHECK(p.silent() == false);
    CHECK(p.bars() == false);
    CHECK(p.error_bidi() == true);
    CHECK(p.error_bleed() == doctest::Approx(0.99f));
    CHECK(p.stability() == doctest::Approx(0.3));
    CHECK_FALSE(p.codec_specs().empty());
}

// --- known profile: 128k ---

TEST_CASE("profile: 128k")
{
    encoding_profile p;
    REQUIRE(encoding_profile::profile_named("128k", p));

    CHECK(p.width() == 512);
    CHECK(p.height() == 342);
    CHECK(p.byterate() == 380);
    CHECK(p.fps_ratio() == 4);
    CHECK(p.silent() == true);
    CHECK(p.dither() == grayscale::dithering::ordered);
}

// --- all profiles loadable ---

TEST_CASE("profile: all known profiles load")
{
    // Names from the profile_table
    std::vector<std::string> names = {"128k", "512k", "xl", "plus", "performer", "portable", "se", "se30", "perfect"};

    for (const auto &name : names)
    {
        CAPTURE(name);
        encoding_profile p;
        CHECK(encoding_profile::profile_named(name, p));
    }
}

// --- unknown profile ---

TEST_CASE("profile: unknown profile returns false")
{
    encoding_profile p;
    CHECK_FALSE(encoding_profile::profile_named("nonexistent", p));
}

// --- profile dimensions ---

TEST_CASE("profile: dimensions valid for all profiles")
{
    std::vector<std::string> names = {"128k", "512k", "xl", "plus", "performer", "portable", "se", "se30", "perfect"};

    for (const auto &name : names)
    {
        CAPTURE(name);
        encoding_profile p;
        REQUIRE(encoding_profile::profile_named(name, p));

        CHECK(p.width() % 32 == 0);
        CHECK(p.height() > 0);
    }
}

// --- codec specs non-empty ---

TEST_CASE("profile: all profiles have codec specs")
{
    std::vector<std::string> names = {"128k", "512k", "xl", "plus", "performer", "portable", "se", "se30", "perfect"};

    for (const auto &name : names)
    {
        CAPTURE(name);
        encoding_profile p;
        REQUIRE(encoding_profile::profile_named(name, p));

        CHECK_FALSE(p.codec_specs().empty());
    }
}

// --- profile description ---

TEST_CASE("profile: description is non-empty")
{
    encoding_profile p;
    REQUIRE(encoding_profile::profile_named("se30", p));

    CHECK_FALSE(p.description().empty());
}
