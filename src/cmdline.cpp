#include "cmdline.hpp"
#include "flimutil.hpp"
#include "grayscale.hpp"
#include <cstring>
#include <format>
#include <iostream>

namespace macflim
{

// Forward declarations
extern const char *version;
extern const std::string temp_file();

void usage(const std::string name)
{
    std::cerr << std::format("Usage\n");
    std::cerr << std::format("{} INPUT [OPTIONS ...]\n", name);
    std::cerr << std::format("  INPUT can be either a mp4 file name, a movie URL or a 'pgm' pattern.'\n");

    std::cerr << std::format("\n  Input options:\n");
    std::cerr << std::format("    --from TIME                 : time offset to start extracting from\n");
    std::cerr << std::format(
        "    --duration TIME             : time duration of the extracted clip (default: full media)\n");
    std::cerr << std::format(
        "    --poster TIME               : frame to extract the poster from (by default 1/3 of duration)ß\n");
    std::cerr << std::format(
        "    --fps FPS                   : for 'pgm' pattern, specifies the framerate to be used\n");
    std::cerr << std::format(
        "    --audio FILE                : for 'pgm', specifices a separate u8 22200 Hz wav file with audio\n");
    std::cerr << std::format("    --srt FILE                  : burns the subtitle file into the flim\n");

    std::cerr << std::format("\n  Output options:\n");
    std::cerr << std::format(
        "    --flim FILE                 : name of the flim file to create (by default 'out.flim')\n");
    std::cerr << std::format("    --mp4 FILE                  : outputs a 60fps mp4 file with the result\n");
    std::cerr << std::format("    --gif FILE                  : outputs a 20fps gif file with the result\n");
    std::cerr << std::format("    --pgm PATTERN               : output every generated grayscale in a pgm file\n");
    std::cerr << std::format("    --pgm-poster PATTERN        : output poster thumbnails (128x86) of input images\n");
    std::cerr << std::format("    --pgm-diff PATTERN          : output difference between encoded result and source\n");
    std::cerr << std::format("    --pgm-change PATTERN        : output difference between consecutive frames\n");
    std::cerr << std::format("    --pgm-target PATTERN        : output target source images\n");

    std::cerr << std::format("\n  Encoding options:\n");
    std::cerr << std::format("    --profile PROFILE           : presents the specific encoding profile, which sets a "
                             "suitable default for all encoding options\n");
    std::cerr << std::format("      Default is 'se30'. See below for description of profiles.\n");
    std::cerr << std::format("    --silent BOOLEAN            : set to true for silent flims\n");
    std::cerr << std::format("    --byterate BYTERATE         : bytes per ticks available for video compression\n");
    std::cerr << std::format("    --fps-ratio BOOLEAN         : ratio of images from the source to drop.\n");
    std::cerr << std::format("    --group BOOLEAN             : if true, packs ticks together to present screen "
                             "updates at the same rate as the input media. Only works on a se30.\n");
    std::cerr << std::format(
        "    --bars BOOLEAN              : if false, grayscale is zoomed in so there are no black bars.\n");
    std::cerr << std::format("    --anchor-x FLOAT            : horizontal anchor point for grayscale extraction "
                             "(0=left, 0.5=center, 1=right)\n");
    std::cerr << std::format("    --anchor-y FLOAT            : vertical anchor point for grayscale extraction (0=top, "
                             "0.5=center, 1=bottom)\n");
    std::cerr << std::format("    --dither DITHER             : specifies the type of dithering to be used.\n");
    std::cerr << std::format("      'ordered' will use a 4x4 ordered dither matrix.\n");
    std::cerr << std::format("      'error' will use an error diffusion algorithm.\n");
    std::cerr << std::format("      'blue' will use blue noise dithering.\n");
    std::cerr << std::format("    --error-algorithm ALGORITHM : error diffusion algorithm to be used\n");
    std::cerr << std::format("      Default 'floyd'. See below for the list of valid error dithering algorithms.\n");
    std::cerr << std::format(
        "    --error-stability FLOAT     : amount of error to be accumulated before changing a screen pixel\n");
    std::cerr << std::format("    --error-bidi BOOLEAN        : if true, error diffusion is applied in different "
                             "direction for even and odd scanlines.\n");
    std::cerr << std::format(
        "    --error-bleed PERCENT       : how much error is moved from a pixel to the neighbours.\n");
    std::cerr << std::format("    --filters FILTERS           : specifies a set of filters to be applied on grayscale "
                             "afgter resizing, but before dithering\n");
    std::cerr << std::format("    --codec CODEC               : adds a specific codec to the encoding. The first "
                             "--codec parameter clears the profile codec list\n");
    std::cerr << std::format("    --initial-frame MODE        : initial frame generation: 'false'=none, "
                             "'optional'=backwards compatible (default), 'true'=required\n");
    std::cerr << std::format("    --loop BOOLEAN              : add trailing frames for perfect loop (default false, "
                             "requires initial frame)\n");

    std::cerr << std::format("\n  Misc options:\n");
    std::cerr << std::format("    --watermark STRING          : adds the string to the upper left corner of the "
                             "generated flim for identification purposes.\n");
    std::cerr << std::format("      use 'auto' to use the encoding parameters as watermark\n");
    std::cerr << std::format("    --debug BOOLEAN             : enables various debug options\n");

    std::cerr << std::format("\nList of profiles names for the --profile option (default 'se30'):\n");
    for (auto n : {"128k", "512k", "xl", "plus", "se", "portable", "se30", "perfect"})
    {
        encoding_profile p;
        encoding_profile::profile_named(n, p);
        std::cerr << std::format("        {} : {}\n", n, p.description());
    }

    std::cerr << std::format(
        "\nList of error diffusion algorithms for the --error_diffusion option (default 'floyd'):\n");

    error_diffusion_algorithms([](const std::string name, const std::string description)
                               { std::cerr << std::format("               {:>16} : {}\n", name, description); });

    std::cerr << std::format("use '{}' --help' for displaying this help page.\n", name);
}

program_options parse_arguments(int argc, char **argv)
{
    program_options opts;
    opts.cache_file = temp_file();

    const std::string cmd_name{argv[0]};

    opts.comment = "FLIM\n";
    for (int i = 0; i != argc; i++)
    {
        if (i != 0)
            opts.comment += " ";
        opts.comment += argv[i];
    }
    opts.comment += "\nflimmaker-version: ";
    opts.comment += version;
    opts.comment += "\n";

    if (!encoding_profile::profile_named(opts.profile_name, opts.custom_profile))
    {
        std::cerr << std::format("Cannot find default profile '{}'\n", opts.profile_name);
        ::exit(EXIT_FAILURE);
    }

    argc--;
    argv++;

    //  If the first argument is a .flim file, switch to flim utility mode
    if (argc > 0 && ends_with(std::string(*argv), ".flim"))
    {
        ::exit(flimutil_main(argc, argv));
    }

    while (argc)
    {
        if (!strcmp(*argv, "--help"))
        {
            usage(cmd_name);
            ::exit(EXIT_SUCCESS);
        }

        if (strncmp(*argv, "--", 2))
        {
            if (opts.input_file != "")
            {
                std::cerr << std::format("Input file specified twice: '{}' and '{}'\n", opts.input_file, *argv);
                ::exit(EXIT_FAILURE);
            }
            opts.input_file = *argv;
        }
        else if (!strcmp(*argv, "--cache"))
        {
            argc--;
            argv++;
            opts.cache_file = *argv;
            opts.generated_cache = false;
        }
        else if (!strcmp(*argv, "--mp4"))
        {
            argc--;
            argv++;
            opts.mp4_file = *argv;
        }
        else if (!strcmp(*argv, "--srt"))
        {
            argc--;
            argv++;
            opts.srt_file = *argv;
        }
        else if (!strcmp(*argv, "--gif"))
        {
            argc--;
            argv++;
            opts.gif_file = *argv;
        }
        else if (!strcmp(*argv, "--profile"))
        {
            argc--;
            argv++;
            opts.profile_name = *argv;
            if (!encoding_profile::profile_named(opts.profile_name, opts.custom_profile))
            {
                std::cerr << "Cannot find encoding profile '" << *argv << "'\n";
                ::exit(EXIT_FAILURE);
            }
        }
        else if (!strcmp(*argv, "--width"))
        {
            argc--;
            argv++;
            opts.width = std::stoi(*argv);
            if ((opts.width % 32) != 0)
            {
                opts.width = (opts.width / 32) * 32;
                std::cerr << "Width must be multiple of 32, rounding it down to '" << opts.width << "'\n";
            }
        }
        else if (!strcmp(*argv, "--height"))
        {
            argc--;
            argv++;
            opts.height = std::stoi(*argv);
        }
        else if (!strcmp(*argv, "--byterate"))
        {
            argc--;
            argv++;
            opts.custom_profile.set_byterate(std::stoi(*argv));
        }
        else if (!strcmp(*argv, "--fps"))
        {
            argc--;
            argv++;
            opts.fps = std::stod(*argv);
        }
        else if (!strcmp(*argv, "--fps-ratio"))
        {
            argc--;
            argv++;
            opts.custom_profile.set_fps_ratio(std::stoi(*argv));
        }
        else if (!strcmp(*argv, "--group"))
        {
            argc--;
            argv++;
            opts.custom_profile.set_group(bool_from(*argv));
        }
        else if (!strcmp(*argv, "--debug"))
        {
            argc--;
            argv++;
            sDebug = bool_from(*argv);
        }
        else if (!strcmp(*argv, "--from"))
        {
            argc--;
            argv++;
            opts.from_index = seconds_from_string(*argv);
        }
        else if (!strcmp(*argv, "--to"))
        {
            argc--;
            argv++;
            opts.to_index = std::stod(*argv);
        }
        else if (!strcmp(*argv, "--duration"))
        {
            argc--;
            argv++;
            opts.duration = seconds_from_string(*argv);
        }
        else if (!strcmp(*argv, "--cover-from"))
        {
            argc--;
            argv++;
            opts.cover_from = atoi(*argv);
        }
        else if (!strcmp(*argv, "--cover-to"))
        {
            argc--;
            argv++;
            opts.cover_to = atoi(*argv);
        }
        else if (!strcmp(*argv, "--cover"))
        {
            argc--;
            argv++;
            opts.cover_from = atoi(*argv);
            opts.cover_to = opts.cover_from + 23;
        }
        else if (!strcmp(*argv, "--poster"))
        {
            argc--;
            argv++;
            opts.poster_ts = seconds_from_string(*argv);
        }
        else if (!strcmp(*argv, "--anchor-x"))
        {
            argc--;
            argv++;
            opts.custom_profile.set_anchor_x(atof(*argv));
        }
        else if (!strcmp(*argv, "--anchor-y"))
        {
            argc--;
            argv++;
            opts.custom_profile.set_anchor_y(atof(*argv));
        }
        else if (!strcmp(*argv, "--audio"))
        {
            argc--;
            argv++;
            opts.audio_file = *argv;
        }
        else if (!strcmp(*argv, "--flim"))
        {
            argc--;
            argv++;
            opts.flim_file = *argv;
        }
        else if (!strcmp(*argv, "--pgm"))
        {
            argc--;
            argv++;
            opts.pgm_pattern = *argv;
        }
        else if (!strcmp(*argv, "--pgm-poster"))
        {
            argc--;
            argv++;
            opts.pgm_poster_pattern = *argv;
        }
        else if (!strcmp(*argv, "--pgm-diff"))
        {
            argc--;
            argv++;
            opts.diff_pattern = *argv;
        }
        else if (!strcmp(*argv, "--pgm-change"))
        {
            argc--;
            argv++;
            opts.change_pattern = *argv;
        }
        else if (!strcmp(*argv, "--pgm-target"))
        {
            argc--;
            argv++;
            opts.target_pattern = *argv;
        }
        else if (!strcmp(*argv, "--comment"))
        {
            argc--;
            argv++;
            opts.comment += "comment: ";
            opts.comment += *argv;
            opts.comment += "\n";
        }
        else if (!strcmp(*argv, "--watermark"))
        {
            argc--;
            argv++;
            if (!strcmp(*argv, "auto"))
                opts.auto_watermark = true;
            else
                opts.watermark = *argv;
        }
        else if (!strcmp(*argv, "--filters"))
        {
            argc--;
            argv++;
            opts.custom_profile.set_filters(*argv);
        }
        else if (!strcmp(*argv, "--bars"))
        {
            argc--;
            argv++;
            opts.custom_profile.set_bars(bool_from(*argv));
        }
        else if (!strcmp(*argv, "--codec"))
        {
            argc--;
            argv++;
            opts.user_codec_specs.push_back(*argv);
        }
        else if (!strcmp(*argv, "--dither"))
        {
            argc--;
            argv++;
            opts.custom_profile.set_dither(*argv);
        }
        else if (!strcmp(*argv, "--error-stability"))
        {
            argc--;
            argv++;
            opts.custom_profile.set_stability(atof(*argv));
        }
        else if (!strcmp(*argv, "--error-algorithm"))
        {
            argc--;
            argv++;
            opts.custom_profile.set_error_algorithm(*argv);
        }
        else if (!strcmp(*argv, "--error-bleed"))
        {
            argc--;
            argv++;
            opts.custom_profile.set_error_bleed(atof(*argv));
        }
        else if (!strcmp(*argv, "--error-bidi"))
        {
            argc--;
            argv++;
            opts.custom_profile.set_error_bidi(bool_from(*argv));
        }
        else if (!strcmp(*argv, "--silent"))
        {
            argc--;
            argv++;
            opts.custom_profile.set_silent(bool_from(*argv));
        }
        else if (!strcmp(*argv, "--initial-frame"))
        {
            argc--;
            argv++;
            if (!opts.custom_profile.set_initial_mode(*argv))
            {
                std::cerr << "Invalid initial-frame mode '" << *argv << "'. Use 'false', 'optional', or 'true'\n";
                ::exit(EXIT_FAILURE);
            }
        }
        else if (!strcmp(*argv, "--loop"))
        {
            argc--;
            argv++;
            opts.custom_profile.set_loop(bool_from(*argv));
        }
        else if (!strcmp(*argv, "--version"))
        {
            std::cout << "flimmaker version " << version << "\n";
            ::exit(EXIT_SUCCESS);
        }
        else
        {
            std::cerr << "Unknown argument " << *argv << "\n";
            ::exit(EXIT_FAILURE);
        }

        argc--;
        argv++;
    }

    // Apply profile's natural dimensions if user didn't override them
    if (opts.width == 0)
        opts.width = opts.custom_profile.width();
    if (opts.height == 0)
        opts.height = opts.custom_profile.height();

    // Update profile with final dimensions (either natural or user-overridden)
    opts.custom_profile.set_size(opts.width, opts.height);

    // If user specified custom codecs, override profile codec specs
    if (opts.user_codec_specs.size() > 0)
    {
        std::vector<std::string> specs = {"null"};
        specs.insert(specs.end(), opts.user_codec_specs.begin(), opts.user_codec_specs.end());
        opts.custom_profile.set_codec_specs(specs);
    }

    if (opts.input_file == "")
    {
        usage(cmd_name);
        ::exit(EXIT_FAILURE);
    }

    return opts;
}

} // namespace macflim
