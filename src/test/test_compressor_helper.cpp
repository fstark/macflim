#include "../compressor_helper.hpp"

#include "../codec_spec.hpp"
#include "../ditherer.hpp"
#include "../dithering_parameters.hpp"
#include "../grayscale.hpp"
#include "../profile.hpp"
#include "../subtitle_burner.hpp"

#include "../doctest.h"

namespace macflim
{

TEST_CASE("compressor_helper: add single grayscale frame")
{
    constexpr size_t W = 512;
    constexpr size_t H = 342;

    std::vector<sound_frame_t> audio(60); // 1 second of audio
    std::vector<codec_spec> codecs = {make_codec("z16", W, H)};

    grayscale prev(W, H);
    fill(prev, 0);

    encoding_profile prof;
    encoding_profile::profile_named("se30", prof);
    dithering_parameters dp = dithering_parameters::from_profile(prof, "");
    ditherer d{prev, dp};
    subtitle_burner sb{{}};

    compressor_helper ch{d, sb, codecs, 30.0, 1000, audio, false};

    grayscale img(W, H);
    fill(img, 128);

    double quality = ch.add(img);

    CHECK(quality >= 0.0);
    CHECK(quality <= 1.0);

    auto frames = ch.get_frames();
    CHECK(frames.size() == 2); // 30fps -> 2 ticks per frame
}

TEST_CASE("compressor_helper: add multiple frames")
{
    constexpr size_t W = 512;
    constexpr size_t H = 342;

    std::vector<sound_frame_t> audio(300); // 5 seconds
    std::vector<codec_spec> codecs = {make_codec("z16", W, H)};

    grayscale prev(W, H);
    fill(prev, 0);

    encoding_profile prof;
    encoding_profile::profile_named("se30", prof);
    dithering_parameters dp = dithering_parameters::from_profile(prof, "");
    ditherer d{prev, dp};
    subtitle_burner sb{{}};

    compressor_helper ch{d, sb, codecs, 30.0, 1000, audio, false};

    // Add 5 frames
    for (int i = 0; i < 5; i++)
    {
        grayscale img(W, H);
        fill(img, i * 50);
        double q = ch.add(img);
        CHECK(q >= 0.0);
        CHECK(q <= 1.0);
    }

    auto frames = ch.get_frames();
    CHECK(frames.size() == 10); // 5 frames * 2 ticks each
}

TEST_CASE("compressor_helper: with z32 codec")
{
    constexpr size_t W = 512;
    constexpr size_t H = 342;

    std::vector<sound_frame_t> audio(60);
    std::vector<codec_spec> codecs = {make_codec("z32", W, H)};

    grayscale prev(W, H);
    fill(prev, 0);

    encoding_profile prof;
    encoding_profile::profile_named("se30", prof);
    dithering_parameters dp = dithering_parameters::from_profile(prof, "");
    ditherer d{prev, dp};
    subtitle_burner sb{{}};

    compressor_helper ch{d, sb, codecs, 30.0, 1000, audio, false};

    grayscale img(W, H);
    fill(img, 128);

    double quality = ch.add(img);
    CHECK(quality >= 0.0);
    CHECK(quality <= 1.0);
}

TEST_CASE("compressor_helper: with multiple codecs")
{
    constexpr size_t W = 512;
    constexpr size_t H = 342;

    std::vector<sound_frame_t> audio(60);
    std::vector<codec_spec> codecs = {make_codec("null", W, H), make_codec("z16", W, H), make_codec("z32", W, H)};

    grayscale prev(W, H);
    fill(prev, 0);

    encoding_profile prof;
    encoding_profile::profile_named("se30", prof);
    dithering_parameters dp = dithering_parameters::from_profile(prof, "");
    ditherer d{prev, dp};
    subtitle_burner sb{{}};

    compressor_helper ch{d, sb, codecs, 30.0, 1000, audio, false};

    grayscale img(W, H);
    fill(img, 128);

    double quality = ch.add(img);
    CHECK(quality >= 0.0);
    CHECK(quality <= 1.0);

    auto frames = ch.get_frames();
    CHECK(frames.size() > 0);
}

TEST_CASE("compressor_helper: group mode packs ticks together")
{
    constexpr size_t W = 512;
    constexpr size_t H = 342;

    std::vector<sound_frame_t> audio(60);
    std::vector<codec_spec> codecs = {make_codec("z16", W, H)};

    grayscale prev(W, H);
    fill(prev, 0);

    encoding_profile prof;
    encoding_profile::profile_named("se30", prof);
    dithering_parameters dp = dithering_parameters::from_profile(prof, "");
    ditherer d{prev, dp};
    subtitle_burner sb{{}};

    // Group mode = true
    compressor_helper ch{d, sb, codecs, 30.0, 1000, audio, true};

    grayscale img(W, H);
    fill(img, 128);

    double quality = ch.add(img);
    CHECK(quality >= 0.0);
    CHECK(quality <= 1.0);

    auto frames = ch.get_frames();
    CHECK(frames.size() == 1); // Group mode: 1 frame with 2 ticks
}

TEST_CASE("compressor_helper: higher byterate allows more data")
{
    constexpr size_t W = 512;
    constexpr size_t H = 342;

    std::vector<sound_frame_t> audio(60);
    std::vector<codec_spec> codecs = {make_codec("z16", W, H)};

    grayscale prev(W, H);
    fill(prev, 0);

    encoding_profile prof;
    encoding_profile::profile_named("se30", prof);
    dithering_parameters dp = dithering_parameters::from_profile(prof, "");
    ditherer d{prev, dp};
    subtitle_burner sb{{}};

    // High byterate
    compressor_helper ch{d, sb, codecs, 30.0, 5000, audio, false};

    grayscale img(W, H);
    fill(img, 128);

    double quality = ch.add(img);
    CHECK(quality >= 0.0);
    CHECK(quality <= 1.0);
}

TEST_CASE("compressor_helper: empty audio vector")
{
    constexpr size_t W = 512;
    constexpr size_t H = 342;

    std::vector<sound_frame_t> audio; // Empty audio
    std::vector<codec_spec> codecs = {make_codec("z16", W, H)};

    grayscale prev(W, H);
    fill(prev, 0);

    encoding_profile prof;
    encoding_profile::profile_named("se30", prof);
    dithering_parameters dp = dithering_parameters::from_profile(prof, "");
    ditherer d{prev, dp};
    subtitle_burner sb{{}};

    compressor_helper ch{d, sb, codecs, 30.0, 1000, audio, false};

    grayscale img(W, H);
    fill(img, 128);

    double quality = ch.add(img);
    CHECK(quality >= 0.0);
    CHECK(quality <= 1.0);

    auto frames = ch.get_frames();
    CHECK(frames.size() > 0);
}

TEST_CASE("compressor_helper: with subtitles")
{
    constexpr size_t W = 512;
    constexpr size_t H = 342;

    std::vector<sound_frame_t> audio(120); // 2 seconds
    std::vector<codec_spec> codecs = {make_codec("z16", W, H)};

    grayscale prev(W, H);
    fill(prev, 0);

    // Create subtitle that appears from 0-1 seconds
    std::vector<subtitle> subs;
    subs.push_back(subtitle{0.0, 1.0, {"Test"}});

    encoding_profile prof;
    encoding_profile::profile_named("se30", prof);
    dithering_parameters dp = dithering_parameters::from_profile(prof, "");
    ditherer d{prev, dp};
    subtitle_burner sb{subs};

    compressor_helper ch{d, sb, codecs, 30.0, 1000, audio, false};

    grayscale img(W, H);
    fill(img, 128);

    double quality = ch.add(img);
    CHECK(quality >= 0.0);
    CHECK(quality <= 1.0);
}

TEST_CASE("compressor_helper: different fps values")
{
    constexpr size_t W = 512;
    constexpr size_t H = 342;

    std::vector<sound_frame_t> audio(60);
    std::vector<codec_spec> codecs = {make_codec("z16", W, H)};

    grayscale prev(W, H);
    fill(prev, 0);

    encoding_profile prof;
    encoding_profile::profile_named("se30", prof);
    dithering_parameters dp = dithering_parameters::from_profile(prof, "");
    ditherer d{prev, dp};
    subtitle_burner sb{{}};

    // Test with 60 fps
    compressor_helper ch{d, sb, codecs, 60.0, 1000, audio, false};

    grayscale img(W, H);
    fill(img, 128);

    double quality = ch.add(img);
    CHECK(quality >= 0.0);
    CHECK(quality <= 1.0);

    auto frames = ch.get_frames();
    CHECK(frames.size() == 1); // 60fps -> 1 tick per frame
}

TEST_CASE("compressor_helper: quality metrics are valid")
{
    constexpr size_t W = 512;
    constexpr size_t H = 342;

    std::vector<sound_frame_t> audio(300);
    std::vector<codec_spec> codecs = {make_codec("z16", W, H)};

    grayscale prev(W, H);
    fill(prev, 0);

    encoding_profile prof;
    encoding_profile::profile_named("se30", prof);
    dithering_parameters dp = dithering_parameters::from_profile(prof, "");
    ditherer d{prev, dp};
    subtitle_burner sb{{}};

    compressor_helper ch{d, sb, codecs, 30.0, 1000, audio, false};

    // Add several frames with different gray values
    for (int i = 0; i < 10; i++)
    {
        grayscale img(W, H);
        fill(img, (i * 25) % 256);
        double q = ch.add(img);

        // Quality should always be in [0, 1]
        CHECK(q >= 0.0);
        CHECK(q <= 1.0);
    }
}

} // namespace macflim
