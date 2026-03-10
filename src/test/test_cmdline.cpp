#include "../cmdline.hpp"
#include "../grayscale.hpp"

#include "../doctest.h"

#include <cstring>

namespace macflim
{

TEST_CASE("cmdline: parse_arguments with minimal input")
{
    char arg0[] = "flimmaker";
    char arg1[] = "input.mp4";
    char *argv[] = {arg0, arg1};

    auto opts = parse_arguments(2, argv);

    CHECK(opts.input_file == "input.mp4");
    CHECK(opts.flim_file == "out.flim");
    CHECK(opts.profile_name == "se30");
    CHECK(opts.fps == 24.0);
}

TEST_CASE("cmdline: parse_arguments with --flim output")
{
    char arg0[] = "flimmaker";
    char arg1[] = "input.mp4";
    char arg2[] = "--flim";
    char arg3[] = "output.flim";
    char *argv[] = {arg0, arg1, arg2, arg3};

    auto opts = parse_arguments(4, argv);

    CHECK(opts.input_file == "input.mp4");
    CHECK(opts.flim_file == "output.flim");
}

TEST_CASE("cmdline: parse_arguments with --profile")
{
    char arg0[] = "flimmaker";
    char arg1[] = "input.mp4";
    char arg2[] = "--profile";
    char arg3[] = "plus";
    char *argv[] = {arg0, arg1, arg2, arg3};

    auto opts = parse_arguments(4, argv);

    CHECK(opts.profile_name == "plus");
    CHECK(opts.custom_profile.width() == 512);
    CHECK(opts.custom_profile.height() == 342);
}

TEST_CASE("cmdline: parse_arguments with --width and --height")
{
    char arg0[] = "flimmaker";
    char arg1[] = "input.mp4";
    char arg2[] = "--width";
    char arg3[] = "512";
    char arg4[] = "--height";
    char arg5[] = "342";
    char *argv[] = {arg0, arg1, arg2, arg3, arg4, arg5};

    auto opts = parse_arguments(6, argv);

    CHECK(opts.width == 512);
    CHECK(opts.height == 342);
}

TEST_CASE("cmdline: parse_arguments with --fps")
{
    char arg0[] = "flimmaker";
    char arg1[] = "input.mp4";
    char arg2[] = "--fps";
    char arg3[] = "30.0";
    char *argv[] = {arg0, arg1, arg2, arg3};

    auto opts = parse_arguments(4, argv);

    CHECK(opts.fps == 30.0);
}

TEST_CASE("cmdline: parse_arguments with --byterate")
{
    char arg0[] = "flimmaker";
    char arg1[] = "input.mp4";
    char arg2[] = "--byterate";
    char arg3[] = "2000";
    char *argv[] = {arg0, arg1, arg2, arg3};

    auto opts = parse_arguments(4, argv);

    CHECK(opts.custom_profile.byterate() == 2000);
}

TEST_CASE("cmdline: parse_arguments with --watermark")
{
    char arg0[] = "flimmaker";
    char arg1[] = "input.mp4";
    char arg2[] = "--watermark";
    char arg3[] = "TEST";
    char *argv[] = {arg0, arg1, arg2, arg3};

    auto opts = parse_arguments(4, argv);

    CHECK(opts.watermark == "TEST");
    CHECK(opts.auto_watermark == false);
}

TEST_CASE("cmdline: parse_arguments with --watermark auto")
{
    char arg0[] = "flimmaker";
    char arg1[] = "input.mp4";
    char arg2[] = "--watermark";
    char arg3[] = "auto";
    char *argv[] = {arg0, arg1, arg2, arg3};

    auto opts = parse_arguments(4, argv);

    CHECK(opts.auto_watermark == true);
}

TEST_CASE("cmdline: parse_arguments with --codec")
{
    char arg0[] = "flimmaker";
    char arg1[] = "input.mp4";
    char arg2[] = "--codec";
    char arg3[] = "z16";
    char arg4[] = "--codec";
    char arg5[] = "z32";
    char *argv[] = {arg0, arg1, arg2, arg3, arg4, arg5};

    auto opts = parse_arguments(6, argv);

    CHECK(opts.user_codec_specs.size() == 2);
    CHECK(opts.user_codec_specs[0] == "z16");
    CHECK(opts.user_codec_specs[1] == "z32");
}

TEST_CASE("cmdline: parse_arguments with --from and --duration")
{
    char arg0[] = "flimmaker";
    char arg1[] = "input.mp4";
    char arg2[] = "--from";
    char arg3[] = "10.5";
    char arg4[] = "--duration";
    char arg5[] = "30";
    char *argv[] = {arg0, arg1, arg2, arg3, arg4, arg5};

    auto opts = parse_arguments(6, argv);

    CHECK(opts.from_index == 10.5);
    CHECK(opts.duration == 30.0);
}

TEST_CASE("cmdline: parse_arguments with --group true")
{
    char arg0[] = "flimmaker";
    char arg1[] = "input.mp4";
    char arg2[] = "--group";
    char arg3[] = "true";
    char *argv[] = {arg0, arg1, arg2, arg3};

    auto opts = parse_arguments(4, argv);

    CHECK(opts.custom_profile.group() == true);
}

TEST_CASE("cmdline: parse_arguments with --group false")
{
    char arg0[] = "flimmaker";
    char arg1[] = "input.mp4";
    char arg2[] = "--group";
    char arg3[] = "false";
    char *argv[] = {arg0, arg1, arg2, arg3};

    auto opts = parse_arguments(4, argv);

    CHECK(opts.custom_profile.group() == false);
}

TEST_CASE("cmdline: parse_arguments with --srt subtitle file")
{
    char arg0[] = "flimmaker";
    char arg1[] = "input.mp4";
    char arg2[] = "--srt";
    char arg3[] = "subtitles.srt";
    char *argv[] = {arg0, arg1, arg2, arg3};

    auto opts = parse_arguments(4, argv);

    CHECK(opts.srt_file == "subtitles.srt");
}

TEST_CASE("cmdline: parse_arguments with --mp4 output")
{
    char arg0[] = "flimmaker";
    char arg1[] = "input.mp4";
    char arg2[] = "--mp4";
    char arg3[] = "output.mp4";
    char *argv[] = {arg0, arg1, arg2, arg3};

    auto opts = parse_arguments(4, argv);

    CHECK(opts.mp4_file == "output.mp4");
}

TEST_CASE("cmdline: parse_arguments with --gif output")
{
    char arg0[] = "flimmaker";
    char arg1[] = "input.mp4";
    char arg2[] = "--gif";
    char arg3[] = "output.gif";
    char *argv[] = {arg0, arg1, arg2, arg3};

    auto opts = parse_arguments(4, argv);

    CHECK(opts.gif_file == "output.gif");
}

TEST_CASE("cmdline: parse_arguments with --fps")
{
    char arg0[] = "flimmaker";
    char arg1[] = "input.mp4";
    char arg2[] = "--fps";
    char arg3[] = "30";
    char *argv[] = {arg0, arg1, arg2, arg3};

    auto opts = parse_arguments(4, argv);

    CHECK(opts.fps == 30.0);
}

TEST_CASE("cmdline: parse_arguments with --from and --duration")
{
    char arg0[] = "flimmaker";
    char arg1[] = "input.mp4";
    char arg2[] = "--from";
    char arg3[] = "00:01:30";
    char arg4[] = "--duration";
    char arg5[] = "00:00:45";
    char *argv[] = {arg0, arg1, arg2, arg3, arg4, arg5};

    auto opts = parse_arguments(6, argv);

    CHECK(opts.from_index == 90); // 1 minute 30 seconds
    CHECK(opts.duration == 45);   // 45 seconds
}

TEST_CASE("cmdline: error - empty input file")
{
    char arg0[] = "flimmaker";
    char *argv[] = {arg0};

    try
    {
        parse_arguments(1, argv);
        FAIL("Should have thrown config_error");
    }
    catch (const config_error &e)
    {
        CHECK(std::string(e.what()).find("No input file") != std::string::npos);
    }
}

TEST_CASE("cmdline: error - unknown profile")
{
    char arg0[] = "flimmaker";
    char arg1[] = "input.mp4";
    char arg2[] = "--profile";
    char arg3[] = "nonexistent";
    char *argv[] = {arg0, arg1, arg2, arg3};

    try
    {
        parse_arguments(4, argv);
        FAIL("Should have thrown config_error");
    }
    catch (const config_error &e)
    {
        CHECK(std::string(e.what()).find("Cannot find encoding profile") != std::string::npos);
        CHECK(e.option() == "nonexistent");
    }
}

TEST_CASE("cmdline: error - unknown flag")
{
    char arg0[] = "flimmaker";
    char arg1[] = "input.mp4";
    char arg2[] = "--unknown-flag";
    char *argv[] = {arg0, arg1, arg2};

    try
    {
        parse_arguments(3, argv);
        FAIL("Should have thrown config_error");
    }
    catch (const config_error &e)
    {
        CHECK(std::string(e.what()).find("Unknown argument") != std::string::npos);
    }
}

TEST_CASE("cmdline: error - duplicate input file")
{
    char arg0[] = "flimmaker";
    char arg1[] = "input1.mp4";
    char arg2[] = "input2.mp4";
    char *argv[] = {arg0, arg1, arg2};

    try
    {
        parse_arguments(3, argv);
        FAIL("Should have thrown config_error");
    }
    catch (const config_error &e)
    {
        CHECK(std::string(e.what()).find("specified twice") != std::string::npos);
    }
}

// Input option tests
TEST_CASE("cmdline: --cache option")
{
    char arg0[] = "flimmaker";
    char arg1[] = "input.mp4";
    char arg2[] = "--cache";
    char arg3[] = "/tmp/cache.mp4";
    char *argv[] = {arg0, arg1, arg2, arg3};

    auto opts = parse_arguments(4, argv);

    CHECK(opts.cache_file == "/tmp/cache.mp4");
    CHECK(opts.generated_cache == false);
}

TEST_CASE("cmdline: --audio option")
{
    char arg0[] = "flimmaker";
    char arg1[] = "input.pgm";
    char arg2[] = "--audio";
    char arg3[] = "audio.wav";
    char *argv[] = {arg0, arg1, arg2, arg3};

    auto opts = parse_arguments(4, argv);

    CHECK(opts.audio_file == "audio.wav");
}

// Output option tests
TEST_CASE("cmdline: --pgm option")
{
    char arg0[] = "flimmaker";
    char arg1[] = "input.mp4";
    char arg2[] = "--pgm";
    char arg3[] = "frame_%04d.pgm";
    char *argv[] = {arg0, arg1, arg2, arg3};

    auto opts = parse_arguments(4, argv);

    CHECK(opts.pgm_pattern == "frame_%04d.pgm");
}

TEST_CASE("cmdline: --pgm-poster option")
{
    char arg0[] = "flimmaker";
    char arg1[] = "input.mp4";
    char arg2[] = "--pgm-poster";
    char arg3[] = "poster_%04d.pgm";
    char *argv[] = {arg0, arg1, arg2, arg3};

    auto opts = parse_arguments(4, argv);

    CHECK(opts.pgm_poster_pattern == "poster_%04d.pgm");
}

TEST_CASE("cmdline: --pgm-diff option")
{
    char arg0[] = "flimmaker";
    char arg1[] = "input.mp4";
    char arg2[] = "--pgm-diff";
    char arg3[] = "diff_%04d.pgm";
    char *argv[] = {arg0, arg1, arg2, arg3};

    auto opts = parse_arguments(4, argv);

    CHECK(opts.diff_pattern == "diff_%04d.pgm");
}

TEST_CASE("cmdline: --pgm-change option")
{
    char arg0[] = "flimmaker";
    char arg1[] = "input.mp4";
    char arg2[] = "--pgm-change";
    char arg3[] = "change_%04d.pgm";
    char *argv[] = {arg0, arg1, arg2, arg3};

    auto opts = parse_arguments(4, argv);

    CHECK(opts.change_pattern == "change_%04d.pgm");
}

TEST_CASE("cmdline: --pgm-target option")
{
    char arg0[] = "flimmaker";
    char arg1[] = "input.mp4";
    char arg2[] = "--pgm-target";
    char arg3[] = "target_%04d.pgm";
    char *argv[] = {arg0, arg1, arg2, arg3};

    auto opts = parse_arguments(4, argv);

    CHECK(opts.target_pattern == "target_%04d.pgm");
}

TEST_CASE("cmdline: --comment option")
{
    char arg0[] = "flimmaker";
    char arg1[] = "input.mp4";
    char arg2[] = "--comment";
    char arg3[] = "My test video";
    char *argv[] = {arg0, arg1, arg2, arg3};

    auto opts = parse_arguments(4, argv);

    CHECK(opts.comment.find("comment: My test video") != std::string::npos);
}

// Encoding option tests
TEST_CASE("cmdline: --byterate option")
{
    char arg0[] = "flimmaker";
    char arg1[] = "input.mp4";
    char arg2[] = "--byterate";
    char arg3[] = "500";
    char *argv[] = {arg0, arg1, arg2, arg3};

    auto opts = parse_arguments(4, argv);

    CHECK(opts.custom_profile.byterate() == 500);
}

TEST_CASE("cmdline: --fps-ratio option")
{
    char arg0[] = "flimmaker";
    char arg1[] = "input.mp4";
    char arg2[] = "--fps-ratio";
    char arg3[] = "2";
    char *argv[] = {arg0, arg1, arg2, arg3};

    auto opts = parse_arguments(4, argv);

    CHECK(opts.custom_profile.fps_ratio() == 2);
}

TEST_CASE("cmdline: --group option")
{
    char arg0[] = "flimmaker";
    char arg1[] = "input.mp4";
    char arg2[] = "--group";
    char arg3[] = "true";
    char *argv[] = {arg0, arg1, arg2, arg3};

    auto opts = parse_arguments(4, argv);

    CHECK(opts.custom_profile.group() == true);
}

TEST_CASE("cmdline: --bars option")
{
    char arg0[] = "flimmaker";
    char arg1[] = "input.mp4";
    char arg2[] = "--bars";
    char arg3[] = "false";
    char *argv[] = {arg0, arg1, arg2, arg3};

    auto opts = parse_arguments(4, argv);

    CHECK(opts.custom_profile.bars() == false);
}

TEST_CASE("cmdline: --anchor-x option")
{
    char arg0[] = "flimmaker";
    char arg1[] = "input.mp4";
    char arg2[] = "--anchor-x";
    char arg3[] = "0.75";
    char *argv[] = {arg0, arg1, arg2, arg3};

    auto opts = parse_arguments(4, argv);

    CHECK(opts.custom_profile.anchor_x() == doctest::Approx(0.75));
}

TEST_CASE("cmdline: --anchor-y option")
{
    char arg0[] = "flimmaker";
    char arg1[] = "input.mp4";
    char arg2[] = "--anchor-y";
    char arg3[] = "0.25";
    char *argv[] = {arg0, arg1, arg2, arg3};

    auto opts = parse_arguments(4, argv);

    CHECK(opts.custom_profile.anchor_y() == doctest::Approx(0.25));
}

TEST_CASE("cmdline: --filters option")
{
    char arg0[] = "flimmaker";
    char arg1[] = "input.mp4";
    char arg2[] = "--filters";
    char arg3[] = "b3s1";
    char *argv[] = {arg0, arg1, arg2, arg3};

    auto opts = parse_arguments(4, argv);

    CHECK(opts.custom_profile.filters() == "b3s1");
}

TEST_CASE("cmdline: --dither option")
{
    char arg0[] = "flimmaker";
    char arg1[] = "input.mp4";
    char arg2[] = "--dither";
    char arg3[] = "ordered";
    char *argv[] = {arg0, arg1, arg2, arg3};

    auto opts = parse_arguments(4, argv);

    CHECK(opts.custom_profile.dither() == grayscale::dithering::ordered);
}

TEST_CASE("cmdline: --error-stability option")
{
    char arg0[] = "flimmaker";
    char arg1[] = "input.mp4";
    char arg2[] = "--error-stability";
    char arg3[] = "0.6";
    char *argv[] = {arg0, arg1, arg2, arg3};

    auto opts = parse_arguments(4, argv);

    CHECK(opts.custom_profile.stability() == doctest::Approx(0.6));
}

TEST_CASE("cmdline: --error-algorithm option")
{
    char arg0[] = "flimmaker";
    char arg1[] = "input.mp4";
    char arg2[] = "--error-algorithm";
    char arg3[] = "atkinson";
    char *argv[] = {arg0, arg1, arg2, arg3};

    auto opts = parse_arguments(4, argv);

    CHECK(opts.custom_profile.error_algorithm() == "atkinson");
}

TEST_CASE("cmdline: --error-bleed option")
{
    char arg0[] = "flimmaker";
    char arg1[] = "input.mp4";
    char arg2[] = "--error-bleed";
    char arg3[] = "0.8";
    char *argv[] = {arg0, arg1, arg2, arg3};

    auto opts = parse_arguments(4, argv);

    CHECK(opts.custom_profile.error_bleed() == doctest::Approx(0.8));
}

TEST_CASE("cmdline: --error-bidi option")
{
    char arg0[] = "flimmaker";
    char arg1[] = "input.mp4";
    char arg2[] = "--error-bidi";
    char arg3[] = "true";
    char *argv[] = {arg0, arg1, arg2, arg3};

    auto opts = parse_arguments(4, argv);

    CHECK(opts.custom_profile.error_bidi() == true);
}

TEST_CASE("cmdline: --silent option")
{
    char arg0[] = "flimmaker";
    char arg1[] = "input.mp4";
    char arg2[] = "--silent";
    char arg3[] = "true";
    char *argv[] = {arg0, arg1, arg2, arg3};

    auto opts = parse_arguments(4, argv);

    CHECK(opts.custom_profile.silent() == true);
}

TEST_CASE("cmdline: --codec option")
{
    char arg0[] = "flimmaker";
    char arg1[] = "input.mp4";
    char arg2[] = "--codec";
    char arg3[] = "xor";
    char *argv[] = {arg0, arg1, arg2, arg3};

    auto opts = parse_arguments(4, argv);

    CHECK(opts.user_codec_specs.size() == 1);
    CHECK(opts.user_codec_specs[0] == "xor");
}

// Dimension tests
TEST_CASE("cmdline: --width option")
{
    char arg0[] = "flimmaker";
    char arg1[] = "input.mp4";
    char arg2[] = "--width";
    char arg3[] = "512";
    char *argv[] = {arg0, arg1, arg2, arg3};

    auto opts = parse_arguments(4, argv);

    CHECK(opts.width == 512);
}

TEST_CASE("cmdline: --height option")
{
    char arg0[] = "flimmaker";
    char arg1[] = "input.mp4";
    char arg2[] = "--height";
    char arg3[] = "342";
    char *argv[] = {arg0, arg1, arg2, arg3};

    auto opts = parse_arguments(4, argv);

    CHECK(opts.height == 342);
}

// Timing tests
TEST_CASE("cmdline: --to option")
{
    char arg0[] = "flimmaker";
    char arg1[] = "input.mp4";
    char arg2[] = "--to";
    char arg3[] = "120.5";
    char *argv[] = {arg0, arg1, arg2, arg3};

    auto opts = parse_arguments(4, argv);

    CHECK(opts.to_index == doctest::Approx(120.5));
}

TEST_CASE("cmdline: --poster option")
{
    char arg0[] = "flimmaker";
    char arg1[] = "input.mp4";
    char arg2[] = "--poster";
    char arg3[] = "00:01:15";
    char *argv[] = {arg0, arg1, arg2, arg3};

    auto opts = parse_arguments(4, argv);

    CHECK(opts.poster_ts == 75);
}

// Cover tests
TEST_CASE("cmdline: --cover option")
{
    char arg0[] = "flimmaker";
    char arg1[] = "input.mp4";
    char arg2[] = "--cover";
    char arg3[] = "10";
    char *argv[] = {arg0, arg1, arg2, arg3};

    auto opts = parse_arguments(4, argv);

    CHECK(opts.cover_from == 10);
    CHECK(opts.cover_to == 33); // cover_from + 23
}

TEST_CASE("cmdline: --cover-from and --cover-to options")
{
    char arg0[] = "flimmaker";
    char arg1[] = "input.mp4";
    char arg2[] = "--cover-from";
    char arg3[] = "5";
    char arg4[] = "--cover-to";
    char arg5[] = "30";
    char *argv[] = {arg0, arg1, arg2, arg3, arg4, arg5};

    auto opts = parse_arguments(6, argv);

    CHECK(opts.cover_from == 5);
    CHECK(opts.cover_to == 30);
}

TEST_CASE("cmdline: --watermark option")
{
    char arg0[] = "flimmaker";
    char arg1[] = "input.mp4";
    char arg2[] = "--watermark";
    char arg3[] = "TEST";
    char *argv[] = {arg0, arg1, arg2, arg3};

    auto opts = parse_arguments(4, argv);

    CHECK(opts.watermark == "TEST");
}

TEST_CASE("cmdline: --watermark auto option")
{
    char arg0[] = "flimmaker";
    char arg1[] = "input.mp4";
    char arg2[] = "--watermark";
    char arg3[] = "auto";
    char *argv[] = {arg0, arg1, arg2, arg3};

    auto opts = parse_arguments(4, argv);

    CHECK(opts.auto_watermark == true);
}

TEST_CASE("cmdline: --srt option")
{
    char arg0[] = "flimmaker";
    char arg1[] = "input.mp4";
    char arg2[] = "--srt";
    char arg3[] = "subtitles.srt";
    char *argv[] = {arg0, arg1, arg2, arg3};

    auto opts = parse_arguments(4, argv);

    CHECK(opts.srt_file == "subtitles.srt");
}

TEST_CASE("cmdline: --loop option")
{
    char arg0[] = "flimmaker";
    char arg1[] = "input.mp4";
    char arg2[] = "--loop";
    char arg3[] = "true";
    char *argv[] = {arg0, arg1, arg2, arg3};

    auto opts = parse_arguments(4, argv);

    CHECK(opts.custom_profile.loop() == true);
}

TEST_CASE("cmdline: --initial-frame optional")
{
    char arg0[] = "flimmaker";
    char arg1[] = "input.mp4";
    char arg2[] = "--initial-frame";
    char arg3[] = "optional";
    char *argv[] = {arg0, arg1, arg2, arg3};

    auto opts = parse_arguments(4, argv);

    CHECK(opts.custom_profile.initial_mode() == initial_frame_mode::optional);
}

TEST_CASE("cmdline: --initial-frame required")
{
    char arg0[] = "flimmaker";
    char arg1[] = "input.mp4";
    char arg2[] = "--initial-frame";
    char arg3[] = "true";
    char *argv[] = {arg0, arg1, arg2, arg3};

    auto opts = parse_arguments(4, argv);

    CHECK(opts.custom_profile.initial_mode() == initial_frame_mode::required);
}

TEST_CASE("cmdline: --flim option")
{
    char arg0[] = "flimmaker";
    char arg1[] = "input.mp4";
    char arg2[] = "--flim";
    char arg3[] = "custom.flim";
    char *argv[] = {arg0, arg1, arg2, arg3};

    auto opts = parse_arguments(4, argv);

    CHECK(opts.flim_file == "custom.flim");
}

TEST_CASE("cmdline: multiple codec specs")
{
    char arg0[] = "flimmaker";
    char arg1[] = "input.mp4";
    char arg2[] = "--codec";
    char arg3[] = "xor";
    char arg4[] = "--codec";
    char arg5[] = "rle";
    char *argv[] = {arg0, arg1, arg2, arg3, arg4, arg5};

    auto opts = parse_arguments(6, argv);

    CHECK(opts.user_codec_specs.size() == 2);
    CHECK(opts.user_codec_specs[0] == "xor");
    CHECK(opts.user_codec_specs[1] == "rle");
}

TEST_CASE("cmdline: --debug option")
{
    char arg0[] = "flimmaker";
    char arg1[] = "input.mp4";
    char arg2[] = "--debug";
    char arg3[] = "true";
    char *argv[] = {arg0, arg1, arg2, arg3};

    auto opts = parse_arguments(4, argv);

    // sDebug is set, but it's an extern variable
    CHECK(sDebug == true);

    // Reset for other tests
    sDebug = false;
}

TEST_CASE("cmdline: combined --from and --to")
{
    char arg0[] = "flimmaker";
    char arg1[] = "input.mp4";
    char arg2[] = "--from";
    char arg3[] = "5.5";
    char arg4[] = "--to";
    char arg5[] = "20.0";
    char *argv[] = {arg0, arg1, arg2, arg3, arg4, arg5};

    auto opts = parse_arguments(6, argv);

    CHECK(opts.from_index == doctest::Approx(5.5));
    CHECK(opts.to_index == doctest::Approx(20.0));
}

TEST_CASE("cmdline: --duration option")
{
    char arg0[] = "flimmaker";
    char arg1[] = "input.mp4";
    char arg2[] = "--from";
    char arg3[] = "10";
    char arg4[] = "--duration";
    char arg5[] = "30";
    char *argv[] = {arg0, arg1, arg2, arg3, arg4, arg5};

    auto opts = parse_arguments(6, argv);

    CHECK(opts.from_index == doctest::Approx(10.0));
    CHECK(opts.duration == doctest::Approx(30.0));
}

TEST_CASE("cmdline: --profile 128k")
{
    char arg0[] = "flimmaker";
    char arg1[] = "input.mp4";
    char arg2[] = "--profile";
    char arg3[] = "128k";
    char *argv[] = {arg0, arg1, arg2, arg3};

    auto opts = parse_arguments(4, argv);

    CHECK(opts.profile_name == "128k");
}

TEST_CASE("cmdline: --profile se30")
{
    char arg0[] = "flimmaker";
    char arg1[] = "input.mp4";
    char arg2[] = "--profile";
    char arg3[] = "se30";
    char *argv[] = {arg0, arg1, arg2, arg3};

    auto opts = parse_arguments(4, argv);

    CHECK(opts.profile_name == "se30");
}

TEST_CASE("cmdline: auto watermark with --cover")
{
    char arg0[] = "flimmaker";
    char arg1[] = "input.mp4";
    char arg2[] = "--watermark";
    char arg3[] = "auto";
    char arg4[] = "--cover";
    char arg5[] = "5";
    char *argv[] = {arg0, arg1, arg2, arg3, arg4, arg5};

    auto opts = parse_arguments(6, argv);

    CHECK(opts.auto_watermark == true);
    CHECK(opts.cover_from == 5);
    CHECK(opts.cover_to == 28); // cover_from + 23
}

TEST_CASE("cmdline: --initial-frame none")
{
    char arg0[] = "flimmaker";
    char arg1[] = "input.mp4";
    char arg2[] = "--initial-frame";
    char arg3[] = "false";
    char *argv[] = {arg0, arg1, arg2, arg3};

    auto opts = parse_arguments(4, argv);

    CHECK(opts.custom_profile.initial_mode() == initial_frame_mode::none);
}

TEST_CASE("cmdline: combined options test")
{
    char arg0[] = "flimmaker";
    char arg1[] = "input.mp4";
    char arg2[] = "--width";
    char arg3[] = "320";
    char arg4[] = "--height";
    char arg5[] = "240";
    char arg6[] = "--fps";
    char arg7[] = "15";
    char arg8[] = "--flim";
    char arg9[] = "output.flim";
    char *argv[] = {arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9};

    auto opts = parse_arguments(10, argv);

    CHECK(opts.width == 320);
    CHECK(opts.height == 240);
    CHECK(opts.fps == doctest::Approx(15.0));
    CHECK(opts.flim_file == "output.flim");
}

TEST_CASE("cmdline: width rounds down to multiple of 32")
{
    char arg0[] = "flimmaker";
    char arg1[] = "input.mp4";
    char arg2[] = "--width";
    char arg3[] = "515"; // Not a multiple of 32
    char arg4[] = "--profile";
    char arg5[] = "se30";
    char *argv[] = {arg0, arg1, arg2, arg3, arg4, arg5};

    auto opts = parse_arguments(6, argv);

    CHECK(opts.width == 512); // Rounded down from 515 to 512 (16*32)
}

TEST_CASE("cmdline: width 100 rounds down to 96")
{
    char arg0[] = "flimmaker";
    char arg1[] = "input.mp4";
    char arg2[] = "--width";
    char arg3[] = "100";
    char arg4[] = "--profile";
    char arg5[] = "plus";
    char *argv[] = {arg0, arg1, arg2, arg3, arg4, arg5};

    auto opts = parse_arguments(6, argv);

    CHECK(opts.width == 96); // Rounded down to nearest multiple of 32
}

TEST_CASE("cmdline: height without width uses profile default")
{
    char arg0[] = "flimmaker";
    char arg1[] = "input.mp4";
    char arg2[] = "--height";
    char arg3[] = "200";
    char *argv[] = {arg0, arg1, arg2, arg3};

    auto opts = parse_arguments(4, argv);

    CHECK(opts.height == 200);
    CHECK(opts.width == 512); // Default for se30 profile
}

TEST_CASE("cmdline: multiple encoding options")
{
    char arg0[] = "flimmaker";
    char arg1[] = "input.mp4";
    char arg2[] = "--byterate";
    char arg3[] = "600";
    char arg4[] = "--group";
    char arg5[] = "true";
    char arg6[] = "--bars";
    char arg7[] = "false";
    char arg8[] = "--silent";
    char arg9[] = "true";
    char *argv[] = {arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9};

    auto opts = parse_arguments(10, argv);

    CHECK(opts.custom_profile.byterate() == 600);
    CHECK(opts.custom_profile.group() == true);
    CHECK(opts.custom_profile.bars() == false);
    CHECK(opts.custom_profile.silent() == true);
}

TEST_CASE("cmdline: --help throws early_exit with message")
{
    char arg0[] = "flimmaker";
    char arg1[] = "--help";
    char *argv[] = {arg0, arg1};

    try
    {
        parse_arguments(2, argv);
        FAIL("Should have thrown early_exit");
    }
    catch (const early_exit &e)
    {
        CHECK(e.exit_code() == EXIT_SUCCESS);
        CHECK(e.has_message() == true);
        CHECK(std::string(e.what()).find("Usage") != std::string::npos);
    }
}

TEST_CASE("cmdline: --version throws early_exit with message")
{
    char arg0[] = "flimmaker";
    char arg1[] = "--version";
    char *argv[] = {arg0, arg1};

    try
    {
        parse_arguments(2, argv);
        FAIL("Should have thrown early_exit");
    }
    catch (const early_exit &e)
    {
        CHECK(e.exit_code() == EXIT_SUCCESS);
        CHECK(e.has_message() == true);
        CHECK(std::string(e.what()).find("flimmaker version") != std::string::npos);
    }
}

} // namespace macflim
