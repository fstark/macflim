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

// --- profiledescription ---

TEST_CASE("profile: description is non-empty")
{
    encoding_profile p;
    REQUIRE(encoding_profile::profile_named("se30", p));

    CHECK_FALSE(p.description().empty());
}

TEST_CASE("profile: description with different dither modes")
{
    encoding_profile p;
    REQUIRE(encoding_profile::profile_named("se30", p));

    // Test error diffusion description
    p.set_dither("error");
    std::string desc_error = p.description();
    CHECK(desc_error.find("--dither error") != std::string::npos);

    // Test ordered dithering description
    p.set_dither("ordered");
    std::string desc_ordered = p.description();
    CHECK(desc_ordered.find("--dither ordered") != std::string::npos);

    // Test blue noise description
    p.set_dither("blue");
    std::string desc_blue = p.description();
    CHECK(desc_blue.find("--dither blue") != std::string::npos);

    // Verify descriptions differ
    CHECK(desc_error != desc_ordered);
    CHECK(desc_ordered != desc_blue);
}

TEST_CASE("profile: description with different initial modes")
{
    encoding_profile p;
    REQUIRE(encoding_profile::profile_named("se30", p));

    p.set_initial_mode("none");
    std::string desc_none = p.description();
    CHECK(desc_none.find("--initial-frame false") != std::string::npos);

    p.set_initial_mode("required");
    std::string desc_req = p.description();
    CHECK(desc_req.find("--initial-frame true") != std::string::npos);

    p.set_initial_mode("optional");
    std::string desc_opt = p.description();
    CHECK(desc_opt.find("--initial-frame optional") != std::string::npos);
}

// --- profile setters ---

TEST_CASE("profile: set_filters")
{
    encoding_profile p;
    REQUIRE(encoding_profile::profile_named("se30", p));

    p.set_filters("b3s");
    CHECK(p.filters() == "b3s");

    p.set_filters("");
    CHECK(p.filters() == "");

    p.set_filters("g2.0fi");
    CHECK(p.filters() == "g2.0fi");
}

TEST_CASE("profile: set_bars")
{
    encoding_profile p;
    REQUIRE(encoding_profile::profile_named("se30", p));

    p.set_bars(true);
    CHECK(p.bars() == true);

    p.set_bars(false);
    CHECK(p.bars() == false);
}

TEST_CASE("profile: set_anchor_x and set_anchor_y")
{
    encoding_profile p;
    REQUIRE(encoding_profile::profile_named("se30", p));

    p.set_anchor_x(0.5);
    CHECK(p.anchor_x() == doctest::Approx(0.5));

    p.set_anchor_x(0.0);
    CHECK(p.anchor_x() == doctest::Approx(0.0));

    p.set_anchor_x(1.0);
    CHECK(p.anchor_x() == doctest::Approx(1.0));

    p.set_anchor_y(0.3);
    CHECK(p.anchor_y() == doctest::Approx(0.3));

    p.set_anchor_y(0.7);
    CHECK(p.anchor_y() == doctest::Approx(0.7));
}

TEST_CASE("profile: set_dither")
{
    encoding_profile p;
    REQUIRE(encoding_profile::profile_named("se30", p));

    p.set_dither(grayscale::dithering::ordered);
    CHECK(p.dither() == grayscale::dithering::ordered);

    p.set_dither(grayscale::dithering::blue_noise);
    CHECK(p.dither() == grayscale::dithering::blue_noise);

    p.set_dither(grayscale::dithering::error_diffusion);
    CHECK(p.dither() == grayscale::dithering::error_diffusion);
}

TEST_CASE("profile: set_dither from string")
{
    encoding_profile p;
    REQUIRE(encoding_profile::profile_named("se30", p));

    p.set_dither("ordered");
    CHECK(p.dither() == grayscale::dithering::ordered);

    p.set_dither("blue");
    CHECK(p.dither() == grayscale::dithering::blue_noise);

    p.set_dither("error");
    CHECK(p.dither() == grayscale::dithering::error_diffusion);

    // Test invalid dither string throws
    CHECK_THROWS_AS(p.set_dither("invalid"), config_error);
    CHECK_THROWS_AS(p.set_dither(""), config_error);
}

TEST_CASE("profile: set_initial_mode from string")
{
    encoding_profile p;
    REQUIRE(encoding_profile::profile_named("se30", p));

    p.set_initial_mode("none");
    CHECK(p.initial_mode() == initial_frame_mode::none);

    p.set_initial_mode("false");
    CHECK(p.initial_mode() == initial_frame_mode::none);

    p.set_initial_mode("optional");
    CHECK(p.initial_mode() == initial_frame_mode::optional);

    p.set_initial_mode("required");
    CHECK(p.initial_mode() == initial_frame_mode::required);

    p.set_initial_mode("true");
    CHECK(p.initial_mode() == initial_frame_mode::required);

    // Test invalid mode string throws
    CHECK_THROWS_AS(p.set_initial_mode("invalid"), config_error);
    CHECK_THROWS_AS(p.set_initial_mode(""), config_error);
}

TEST_CASE("profile: set_error_algorithm")
{
    encoding_profile p;
    REQUIRE(encoding_profile::profile_named("se30", p));

    p.set_error_algorithm("floyd-steinberg");
    CHECK(p.error_algorithm() == "floyd-steinberg");

    p.set_error_algorithm("atkinson");
    CHECK(p.error_algorithm() == "atkinson");

    p.set_error_algorithm("");
    CHECK(p.error_algorithm() == "");
}

TEST_CASE("profile: set_error_bleed")
{
    encoding_profile p;
    REQUIRE(encoding_profile::profile_named("se30", p));

    p.set_error_bleed(0.5f);
    CHECK(p.error_bleed() == doctest::Approx(0.5f));

    p.set_error_bleed(1.0f);
    CHECK(p.error_bleed() == doctest::Approx(1.0f));

    p.set_error_bleed(0.0f);
    CHECK(p.error_bleed() == doctest::Approx(0.0f));
}

TEST_CASE("profile: set_error_bidi")
{
    encoding_profile p;
    REQUIRE(encoding_profile::profile_named("se30", p));

    p.set_error_bidi(true);
    CHECK(p.error_bidi() == true);

    p.set_error_bidi(false);
    CHECK(p.error_bidi() == false);
}

TEST_CASE("profile: set_stability")
{
    encoding_profile p;
    REQUIRE(encoding_profile::profile_named("se30", p));

    p.set_stability(0.5);
    CHECK(p.stability() == doctest::Approx(0.5));

    p.set_stability(0.0);
    CHECK(p.stability() == doctest::Approx(0.0));

    p.set_stability(1.0);
    CHECK(p.stability() == doctest::Approx(1.0));
}

TEST_CASE("profile: set_codec_specs")
{
    encoding_profile p;
    REQUIRE(encoding_profile::profile_named("se30", p));

    std::vector<std::string> specs = {"z16", "z32", "invert"};
    p.set_codec_specs(specs);
    CHECK(p.codec_specs() == specs);

    std::vector<std::string> empty;
    p.set_codec_specs(empty);
    CHECK(p.codec_specs().empty());
}

TEST_CASE("profile: set_silent")
{
    encoding_profile p;
    REQUIRE(encoding_profile::profile_named("se30", p));

    p.set_silent(true);
    CHECK(p.silent() == true);

    p.set_silent(false);
    CHECK(p.silent() == false);
}

TEST_CASE("profile: set_width")
{
    encoding_profile p;
    REQUIRE(encoding_profile::profile_named("se30", p));

    p.set_width(640);
    CHECK(p.width() == 640);
    CHECK(p.height() == 342); // Height unchanged

    p.set_width(320);
    CHECK(p.width() == 320);
}

TEST_CASE("profile: set_height")
{
    encoding_profile p;
    REQUIRE(encoding_profile::profile_named("se30", p));

    p.set_height(480);
    CHECK(p.height() == 480);
    CHECK(p.width() == 512); // Width unchanged

    p.set_height(240);
    CHECK(p.height() == 240);
}

TEST_CASE("profile: set_byterate")
{
    encoding_profile p;
    REQUIRE(encoding_profile::profile_named("se30", p));

    p.set_byterate(8000);
    CHECK(p.byterate() == 8000);

    p.set_byterate(1000);
    CHECK(p.byterate() == 1000);

    p.set_byterate(0);
    CHECK(p.byterate() == 0);
}

TEST_CASE("profile: set_fps_ratio")
{
    encoding_profile p;
    REQUIRE(encoding_profile::profile_named("se30", p));

    p.set_fps_ratio(2);
    CHECK(p.fps_ratio() == 2);

    p.set_fps_ratio(4);
    CHECK(p.fps_ratio() == 4);

    p.set_fps_ratio(1);
    CHECK(p.fps_ratio() == 1);
}

TEST_CASE("profile: set_group")
{
    encoding_profile p;
    REQUIRE(encoding_profile::profile_named("se30", p));

    p.set_group(false);
    CHECK(p.group() == false);

    p.set_group(true);
    CHECK(p.group() == true);
}

TEST_CASE("profile: set_initial_mode with enum")
{
    encoding_profile p;
    REQUIRE(encoding_profile::profile_named("se30", p));

    p.set_initial_mode(initial_frame_mode::none);
    CHECK(p.initial_mode() == initial_frame_mode::none);

    p.set_initial_mode(initial_frame_mode::optional);
    CHECK(p.initial_mode() == initial_frame_mode::optional);

    p.set_initial_mode(initial_frame_mode::required);
    CHECK(p.initial_mode() == initial_frame_mode::required);
}

TEST_CASE("profile: set_loop")
{
    encoding_profile p;
    REQUIRE(encoding_profile::profile_named("se30", p));

    p.set_loop(true);
    CHECK(p.loop() == true);

    p.set_loop(false);
    CHECK(p.loop() == false);
}
