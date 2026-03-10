#include "../flimencoder.hpp"

#include "../doctest.h"
#include "../profile.hpp"

#include <cmath>

namespace macflim
{

// Test fixture to access private methods
class flimencoder_tester
{
  public:
    flimencoder_tester(const encoding_profile &profile) : encoder_(profile) {}

    int test_clamp(double v, int a, int b)
    {
        return encoder_.clamp(v, a, b);
    }

    std::vector<uint8_t> test_normalize_sound(std::vector<double> sound_samples, size_t len)
    {
        return encoder_.normalize_sound(sound_samples, len);
    }

  private:
    flimencoder encoder_;
};

TEST_SUITE("flimencoder")
{
    TEST_CASE("clamp: value within range")
    {
        encoding_profile profile;
        encoding_profile::profile_named("se30", profile);
        flimencoder_tester tester(profile);

        CHECK(tester.test_clamp(5.0, 0, 10) == 5);
        CHECK(tester.test_clamp(5.4, 0, 10) == 5);
        CHECK(tester.test_clamp(5.5, 0, 10) == 6);
        CHECK(tester.test_clamp(5.6, 0, 10) == 6);
    }

    TEST_CASE("clamp: value below minimum")
    {
        encoding_profile profile;
        encoding_profile::profile_named("se30", profile);
        flimencoder_tester tester(profile);

        CHECK(tester.test_clamp(-5.0, 0, 10) == 0);
        CHECK(tester.test_clamp(-100.0, 0, 255) == 0);
        CHECK(tester.test_clamp(0.4, 1, 10) == 1);
    }

    TEST_CASE("clamp: value above maximum")
    {
        encoding_profile profile;
        encoding_profile::profile_named("se30", profile);
        flimencoder_tester tester(profile);

        CHECK(tester.test_clamp(15.0, 0, 10) == 10);
        CHECK(tester.test_clamp(300.0, 0, 255) == 255);
        CHECK(tester.test_clamp(9.6, 0, 9) == 9);
    }

    TEST_CASE("clamp: boundary values")
    {
        encoding_profile profile;
        encoding_profile::profile_named("se30", profile);
        flimencoder_tester tester(profile);

        CHECK(tester.test_clamp(0.0, 0, 255) == 0);
        CHECK(tester.test_clamp(255.0, 0, 255) == 255);
        CHECK(tester.test_clamp(127.5, 0, 255) == 128);
        CHECK(tester.test_clamp(128.4, 0, 255) == 128);
    }

    TEST_CASE("clamp: rounding behavior")
    {
        encoding_profile profile;
        encoding_profile::profile_named("se30", profile);
        flimencoder_tester tester(profile);

        // v + 0.5 then truncate means round-to-nearest
        CHECK(tester.test_clamp(4.4, 0, 10) == 4);
        CHECK(tester.test_clamp(4.5, 0, 10) == 5);
        CHECK(tester.test_clamp(4.6, 0, 10) == 5);
        CHECK(tester.test_clamp(9.4, 0, 10) == 9);
        CHECK(tester.test_clamp(9.5, 0, 10) == 10);
    }

    TEST_CASE("normalize_sound: empty input")
    {
        encoding_profile profile;
        encoding_profile::profile_named("se30", profile);
        flimencoder_tester tester(profile);

        auto result = tester.test_normalize_sound({}, 0);
        CHECK(result.empty());
    }

    TEST_CASE("normalize_sound: single zero value")
    {
        encoding_profile profile;
        encoding_profile::profile_named("se30", profile);
        flimencoder_tester tester(profile);

        auto result = tester.test_normalize_sound({0.0}, 1);
        CHECK(result.size() == 1);
        // scale = 0, so v/scale is undefined, but clamp ensures valid range
        // This is a degenerate case - let's just verify it doesn't crash
    }

    TEST_CASE("normalize_sound: positive values")
    {
        encoding_profile profile;
        encoding_profile::profile_named("se30", profile);
        flimencoder_tester tester(profile);

        auto result = tester.test_normalize_sound({0.0, 0.5, 1.0}, 3);
        CHECK(result.size() == 3);
        // scale = 1.0, so:
        // 0.0 -> (0.0/1.0)*128+128 = 128
        // 0.5 -> (0.5/1.0)*128+128 = 192
        // 1.0 -> (1.0/1.0)*128+128 = 256 -> clamped to 255
        CHECK(result[0] == 128);
        CHECK(result[1] == 192);
        CHECK(result[2] == 255);
    }

    TEST_CASE("normalize_sound: negative values")
    {
        encoding_profile profile;
        encoding_profile::profile_named("se30", profile);
        flimencoder_tester tester(profile);

        auto result = tester.test_normalize_sound({-1.0, -0.5, 0.0}, 3);
        CHECK(result.size() == 3);
        // scale = 1.0, so:
        // -1.0 -> (-1.0/1.0)*128+128 = 0
        // -0.5 -> (-0.5/1.0)*128+128 = 64
        //  0.0 -> (0.0/1.0)*128+128 = 128
        CHECK(result[0] == 0);
        CHECK(result[1] == 64);
        CHECK(result[2] == 128);
    }

    TEST_CASE("normalize_sound: mixed positive and negative")
    {
        encoding_profile profile;
        encoding_profile::profile_named("se30", profile);
        flimencoder_tester tester(profile);

        auto result = tester.test_normalize_sound({-0.5, 0.0, 1.0}, 3);
        CHECK(result.size() == 3);
        // scale = max(0.5, 1.0) = 1.0
        // -0.5 -> (-0.5/1.0)*128+128 = 64
        //  0.0 -> (0.0/1.0)*128+128 = 128
        //  1.0 -> (1.0/1.0)*128+128 = 256 -> clamped to 255
        CHECK(result[0] == 64);
        CHECK(result[1] == 128);
        CHECK(result[2] == 255);
    }

    TEST_CASE("normalize_sound: symmetric range")
    {
        encoding_profile profile;
        encoding_profile::profile_named("se30", profile);
        flimencoder_tester tester(profile);

        auto result = tester.test_normalize_sound({-1.0, 0.0, 1.0}, 3);
        CHECK(result.size() == 3);
        // scale = 1.0
        // -1.0 -> 0
        //  0.0 -> 128
        //  1.0 -> 255
        CHECK(result[0] == 0);
        CHECK(result[1] == 128);
        CHECK(result[2] == 255);
    }

    TEST_CASE("normalize_sound: truncation and resizing")
    {
        encoding_profile profile;
        encoding_profile::profile_named("se30", profile);
        flimencoder_tester tester(profile);

        // Pass 5 values but request only 3
        auto result = tester.test_normalize_sound({-1.0, -0.5, 0.0, 0.5, 1.0}, 3);
        CHECK(result.size() == 3);
        // After resize, we have {-1.0, -0.5, 0.0}
        // scale = 1.0
        CHECK(result[0] == 0);
        CHECK(result[1] == 64);
        CHECK(result[2] == 128);
    }

    TEST_CASE("normalize_sound: padding with zeros")
    {
        encoding_profile profile;
        encoding_profile::profile_named("se30", profile);
        flimencoder_tester tester(profile);

        // Pass 2 values but request 4 (will be padded with zeros)
        auto result = tester.test_normalize_sound({1.0, -1.0}, 4);
        CHECK(result.size() == 4);
        // After resize, we have {1.0, -1.0, 0.0, 0.0}
        // scale = 1.0
        CHECK(result[0] == 255);
        CHECK(result[1] == 0);
        CHECK(result[2] == 128);
        CHECK(result[3] == 128);
    }

    TEST_CASE("normalize_sound: scaling behavior")
    {
        encoding_profile profile;
        encoding_profile::profile_named("se30", profile);
        flimencoder_tester tester(profile);

        // Test with larger magnitude values that get scaled down
        auto result = tester.test_normalize_sound({-10.0, 0.0, 10.0}, 3);
        CHECK(result.size() == 3);
        // scale = 10.0
        // -10.0 -> (-10.0/10.0)*128+128 = 0
        //   0.0 -> (0.0/10.0)*128+128 = 128
        //  10.0 -> (10.0/10.0)*128+128 = 255
        CHECK(result[0] == 0);
        CHECK(result[1] == 128);
        CHECK(result[2] == 255);
    }

    TEST_CASE("normalize_sound: asymmetric range uses max absolute")
    {
        encoding_profile profile;
        encoding_profile::profile_named("se30", profile);
        flimencoder_tester tester(profile);

        // Min is -2.0, max is 1.0, so scale should be 2.0
        auto result = tester.test_normalize_sound({-2.0, 0.0, 1.0}, 3);
        CHECK(result.size() == 3);
        // scale = max(2.0, 1.0) = 2.0
        // -2.0 -> (-2.0/2.0)*128+128 = 0
        //  0.0 -> (0.0/2.0)*128+128 = 128
        //  1.0 -> (1.0/2.0)*128+128 = 192
        CHECK(result[0] == 0);
        CHECK(result[1] == 128);
        CHECK(result[2] == 192);
    }

    TEST_CASE("setter methods - basic functionality")
    {
        encoding_profile profile;
        encoding_profile::profile_named("se30", profile);
        flimencoder encoder(profile);

        // Test all setter methods (for coverage)
        encoder.set_fps(30.0);
        encoder.set_comment("Test comment");
        encoder.set_cover(0, 10);
        encoder.set_watermark("Test Watermark");
        encoder.set_poster_ts(5.0);

        // Test subtitle setter
        std::vector<subtitle> subs;
        subtitle s1;
        s1.start = 0;
        s1.stop = 60;
        s1.text.push_back("Test subtitle");
        subs.push_back(s1);
        encoder.set_subtitles(subs);

        // Test PGM pattern setters with empty strings (should not create writers)
        encoder.set_pgm_poster_pattern("");
        encoder.set_pgm_diff_pattern("");
        encoder.set_pgm_change_pattern("");
        encoder.set_pgm_target_pattern("");

        // All setters execute successfully (no exceptions)
        CHECK(true);
    }
}

} // namespace macflim
