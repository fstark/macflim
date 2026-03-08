#include "cmdline.hpp"

#include "errors.hpp"
#include "flimutil.hpp"
#include "grayscale.hpp"

#include <cstring>
#include <format>
#include <functional>
#include <iostream>
#include <string_view>
#include <unordered_map>

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

    error_diffusion_algorithms([](std::string_view name, std::string_view description)
                               { std::cerr << std::format("               {:>16} : {}\n", name, description); });

    std::cerr << std::format("use '{}' --help' for displaying this help page.\n", name);
}

/// Simple iterator over argc/argv that safely consumes arguments.
class arg_iterator
{
    int argc_;
    char **argv_;

  public:
    arg_iterator(int argc, char **argv) : argc_{argc}, argv_{argv} {}

    [[nodiscard]] bool has_next() const
    {
        return argc_ > 0;
    }

    /// Return current arg and advance. Throws if exhausted.
    std::string_view next()
    {
        if (argc_ <= 0)
            throw flim_error("Expected argument but reached end of command line");
        std::string_view result = *argv_;
        argc_--;
        argv_++;
        return result;
    }

    /// Return current arg without advancing.
    [[nodiscard]] std::string_view peek() const
    {
        if (argc_ <= 0)
            throw flim_error("Expected argument but reached end of command line");
        return *argv_;
    }

    /// Consume and return the next argument value (the one after a flag).
    std::string_view next_value()
    {
        if (argc_ <= 0)
            throw flim_error("Expected value after flag but reached end of command line");
        return next();
    }
};

using flag_handler = std::function<void(arg_iterator &, program_options &)>;

std::string build_comment_string(int argc, char **argv)
{
    std::string comment = "FLIM\n";
    for (int i = 0; i != argc; i++)
    {
        if (i != 0)
            comment += " ";
        comment += argv[i];
    }
    comment += "\nflimmaker-version: ";
    comment += version;
    comment += "\n";
    return comment;
}

void validate_and_finalize(program_options &opts, const std::string &cmd_name)
{
    // Apply profile's natural dimensions if user didn't override them
    if (opts.width == 0)
        opts.width = opts.custom_profile.width();
    if (opts.height == 0)
        opts.height = opts.custom_profile.height();

    // Update profile with final dimensions (either natural or user-overridden)
    opts.custom_profile.set_size(opts.width, opts.height);

    // If user specified custom codecs, override profile codec specs
    if (!opts.user_codec_specs.empty())
    {
        std::vector<std::string> specs = {"null"};
        specs.insert(specs.end(), opts.user_codec_specs.begin(), opts.user_codec_specs.end());
        opts.custom_profile.set_codec_specs(specs);
    }

    if (opts.input_file.empty())
    {
        usage(cmd_name);
        ::exit(EXIT_FAILURE);
    }
}

/// Build the dispatch table mapping --flag to handler.
/// Each handler consumes its value from the iterator.
const std::unordered_map<std::string_view, flag_handler> &flag_dispatch_table()
{
    static const std::unordered_map<std::string_view, flag_handler> table = {
        // --- Simple string assignments ---
        {"--cache",
         [](arg_iterator &args, program_options &opts)
         {
             opts.cache_file = std::string(args.next_value());
             opts.generated_cache = false;
         }},
        {"--mp4", [](arg_iterator &args, program_options &opts) { opts.mp4_file = std::string(args.next_value()); }},
        {"--srt", [](arg_iterator &args, program_options &opts) { opts.srt_file = std::string(args.next_value()); }},
        {"--gif", [](arg_iterator &args, program_options &opts) { opts.gif_file = std::string(args.next_value()); }},
        {"--audio",
         [](arg_iterator &args, program_options &opts) { opts.audio_file = std::string(args.next_value()); }},
        {"--flim",
         [](arg_iterator &args, program_options &opts) { opts.flim_file = std::string(args.next_value()); }},
        {"--pgm",
         [](arg_iterator &args, program_options &opts) { opts.pgm_pattern = std::string(args.next_value()); }},
        {"--pgm-poster",
         [](arg_iterator &args, program_options &opts)
         { opts.pgm_poster_pattern = std::string(args.next_value()); }},
        {"--pgm-diff",
         [](arg_iterator &args, program_options &opts) { opts.diff_pattern = std::string(args.next_value()); }},
        {"--pgm-change",
         [](arg_iterator &args, program_options &opts) { opts.change_pattern = std::string(args.next_value()); }},
        {"--pgm-target",
         [](arg_iterator &args, program_options &opts) { opts.target_pattern = std::string(args.next_value()); }},
        {"--comment",
         [](arg_iterator &args, program_options &opts)
         {
             opts.comment += "comment: ";
             opts.comment += args.next_value();
             opts.comment += "\n";
         }},

        // --- Profile field setters ---
        {"--byterate",
         [](arg_iterator &args, program_options &opts)
         { opts.custom_profile.set_byterate(std::stoi(std::string(args.next_value()))); }},
        {"--fps",
         [](arg_iterator &args, program_options &opts) { opts.fps = std::stod(std::string(args.next_value())); }},
        {"--fps-ratio",
         [](arg_iterator &args, program_options &opts)
         { opts.custom_profile.set_fps_ratio(std::stoi(std::string(args.next_value()))); }},
        {"--group",
         [](arg_iterator &args, program_options &opts)
         { opts.custom_profile.set_group(bool_from(args.next_value())); }},
        {"--bars",
         [](arg_iterator &args, program_options &opts)
         { opts.custom_profile.set_bars(bool_from(args.next_value())); }},
        {"--anchor-x",
         [](arg_iterator &args, program_options &opts)
         { opts.custom_profile.set_anchor_x(atof(std::string(args.next_value()).c_str())); }},
        {"--anchor-y",
         [](arg_iterator &args, program_options &opts)
         { opts.custom_profile.set_anchor_y(atof(std::string(args.next_value()).c_str())); }},
        {"--filters",
         [](arg_iterator &args, program_options &opts)
         { opts.custom_profile.set_filters(std::string(args.next_value())); }},
        {"--dither",
         [](arg_iterator &args, program_options &opts)
         { opts.custom_profile.set_dither(std::string(args.next_value())); }},
        {"--error-stability",
         [](arg_iterator &args, program_options &opts)
         { opts.custom_profile.set_stability(atof(std::string(args.next_value()).c_str())); }},
        {"--error-algorithm",
         [](arg_iterator &args, program_options &opts)
         { opts.custom_profile.set_error_algorithm(std::string(args.next_value())); }},
        {"--error-bleed",
         [](arg_iterator &args, program_options &opts)
         { opts.custom_profile.set_error_bleed(atof(std::string(args.next_value()).c_str())); }},
        {"--error-bidi",
         [](arg_iterator &args, program_options &opts)
         { opts.custom_profile.set_error_bidi(bool_from(args.next_value())); }},
        {"--silent",
         [](arg_iterator &args, program_options &opts)
         { opts.custom_profile.set_silent(bool_from(args.next_value())); }},
        {"--codec",
         [](arg_iterator &args, program_options &opts)
         { opts.user_codec_specs.push_back(std::string(args.next_value())); }},

        // --- Special-case handlers ---
        {"--debug",
         [](arg_iterator &args, program_options &) { sDebug = bool_from(args.next_value()); }},
        {"--profile",
         [](arg_iterator &args, program_options &opts)
         {
             opts.profile_name = std::string(args.next_value());
             if (!encoding_profile::profile_named(opts.profile_name, opts.custom_profile))
             {
                 std::cerr << std::format("Cannot find encoding profile '{}'\n", opts.profile_name);
                 ::exit(EXIT_FAILURE);
             }
         }},
        {"--width",
         [](arg_iterator &args, program_options &opts)
         {
             opts.width = std::stoi(std::string(args.next_value()));
             if ((opts.width % 32) != 0)
             {
                 opts.width = (opts.width / 32) * 32;
                 std::cerr << std::format("Width must be multiple of 32, rounding it down to '{}'\n", opts.width);
             }
         }},
        {"--height",
         [](arg_iterator &args, program_options &opts)
         { opts.height = std::stoi(std::string(args.next_value())); }},
        {"--from",
         [](arg_iterator &args, program_options &opts) { opts.from_index = seconds_from_string(args.next_value()); }},
        {"--to",
         [](arg_iterator &args, program_options &opts)
         { opts.to_index = std::stod(std::string(args.next_value())); }},
        {"--duration",
         [](arg_iterator &args, program_options &opts) { opts.duration = seconds_from_string(args.next_value()); }},
        {"--cover-from",
         [](arg_iterator &args, program_options &opts)
         { opts.cover_from = atoi(std::string(args.next_value()).c_str()); }},
        {"--cover-to",
         [](arg_iterator &args, program_options &opts)
         { opts.cover_to = atoi(std::string(args.next_value()).c_str()); }},
        {"--cover",
         [](arg_iterator &args, program_options &opts)
         {
             opts.cover_from = atoi(std::string(args.next_value()).c_str());
             opts.cover_to = opts.cover_from + 23;
         }},
        {"--poster",
         [](arg_iterator &args, program_options &opts) { opts.poster_ts = seconds_from_string(args.next_value()); }},
        {"--watermark",
         [](arg_iterator &args, program_options &opts)
         {
             auto val = args.next_value();
             if (val == "auto")
                 opts.auto_watermark = true;
             else
                 opts.watermark = std::string(val);
         }},
        {"--initial-frame",
         [](arg_iterator &args, program_options &opts)
         {
             auto val = std::string(args.next_value());
             try
             {
                 opts.custom_profile.set_initial_mode(val);
             }
             catch (const config_error &)
             {
                 std::cerr << std::format(
                     "Invalid initial-frame mode '{}'. Use 'false', 'optional', or 'true'\n", val);
                 ::exit(EXIT_FAILURE);
             }
         }},
        {"--loop",
         [](arg_iterator &args, program_options &opts)
         { opts.custom_profile.set_loop(bool_from(args.next_value())); }},
        {"--version",
         []([[maybe_unused]] arg_iterator &args, [[maybe_unused]] program_options &opts)
         {
             std::cout << "flimmaker version " << version << "\n";
             ::exit(EXIT_SUCCESS);
         }},
    };
    return table;
}

program_options parse_arguments(int argc, char **argv)
{
    program_options opts;
    opts.cache_file = temp_file();
    const std::string cmd_name{argv[0]};
    opts.comment = build_comment_string(argc, argv);

    if (!encoding_profile::profile_named(opts.profile_name, opts.custom_profile))
    {
        std::cerr << std::format("Cannot find default profile '{}'\n", opts.profile_name);
        ::exit(EXIT_FAILURE);
    }

    argc--;
    argv++;

    // If the first argument is a .flim file, switch to flim utility mode
    if (argc > 0 && ends_with(std::string(*argv), ".flim"))
        ::exit(flimutil_main(argc, argv));

    const auto &dispatch = flag_dispatch_table();
    arg_iterator args(argc, argv);

    while (args.has_next())
    {
        auto arg = args.next();

        if (arg == "--help")
        {
            usage(cmd_name);
            ::exit(EXIT_SUCCESS);
        }

        // Positional argument (no -- prefix) = input file
        if (arg.substr(0, 2) != "--")
        {
            if (!opts.input_file.empty())
            {
                std::cerr << std::format("Input file specified twice: '{}' and '{}'\n", opts.input_file, arg);
                ::exit(EXIT_FAILURE);
            }
            opts.input_file = std::string(arg);
            continue;
        }

        auto it = dispatch.find(arg);
        if (it == dispatch.end())
        {
            std::cerr << std::format("Unknown argument {}\n", arg);
            ::exit(EXIT_FAILURE);
        }

        it->second(args, opts);
    }

    validate_and_finalize(opts, cmd_name);
    return opts;
}

} // namespace macflim
