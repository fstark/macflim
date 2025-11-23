/**
 * The flimmaker tool takes a set of pgm images and generate a flim file suitable for playback by MacFlim on a vintage Mac
 */

/**
 * TODO:
 *      Pixel aging
 *      Cycle budget
 *      => Multiple codecs
 *      => Codec testing tools (flim generation)
 *      New codecs:
 *          Invert rect?
 *          Fill black/white, vertical/horizontal, 8, 16, 32
 *          Fill constant, vertical/horizontal, 8, 16, 32
 *      Works from arbitrary images size (incl : letterbox)
 *      Manages ffmpeg worker / mediainfo / sox
 *      Automatic grid.mp4 generation
 *      flimutil
 */

#include <stdlib.h>
#ifndef _WIN32
#include <unistd.h>
#include <signal.h>
#include <execinfo.h>
#endif
#include <memory.h>
#include <math.h>
#include <assert.h>
#include <limits>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <vector>
#include <array>
#include <memory>
#include <filesystem>
#define noLZG
#ifdef LZG
#include "lzg.h"
#endif
#include "common.hpp"
#include "flimencoder.hpp"

using namespace std::string_literals;

// True if the global '-g' option was set
bool sDebug = false;

// If defined, we add a "stamp" to each stream, to know where it is coming from
#define noSTAMP

#ifdef STAMP
static int sStream = 0;
#endif

#include "image.hpp"
#include "reader.hpp"
#include "filesystem_reader.hpp"
#include "ffmpeg_reader.hpp"
#include "writer.hpp"
#include "subtitles.hpp"

inline bool ends_with(std::string const & value, std::string const & ending)
{
    if (ending.size() > value.size())
        return false;

    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

int num_from_string(const char **s)
{
    int n = 0;
    while (**s >= '0' && **s <= '9')
    {
        n = n * 10 + (**s - '0');
        (*s)++;
    }
    return n;
}

// Converts a timestamp into a second count
// 42 => 42
// 05:31 => 331
// 2:4 => 124
// 02:04.470 => 124.47
// 1230.2 => 1230.2
// 0001:1:1:3.1toto => 219663.1
timestamp_t seconds_from_string(const char *s)
{
    double d = 0;
    for (;;)
    {
        if (*s >= '0' && *s <= '9')
            d = d * 60 + num_from_string(&s);
        if (*s != ':')
            break;
        s++;
    }
    if (!*s)
        return d;
    if (*s == '.')
    {
        double f = 1;
        s++;
        while (*s >= '0' && *s <= '9')
        {
            f /= 10;
            d += f * (*s++ - '0');
        }
    }
    return d;
}

void test_seconds_from_string()
{
    assert(seconds_from_string("42") == 42);
    assert(seconds_from_string("05:31") == 331);
    assert(seconds_from_string("2:4") == 124);
    assert(seconds_from_string("02:04.470") == 124.47);
    assert(seconds_from_string("1230.2") == 1230.2);
    assert(seconds_from_string("0001:1:1:3.1toto") == 219663.1);
}

// Write a bunch of bytes in a file
void write_data(const char *file, u_int8_t *data, size_t len)
{
    FILE *f = fopen(file, "wb");
    while (len--)
        fputc(*data++, f);
    fclose(f);
}

const char *version = "2.0.0";

#include <iostream>
#include <chrono>
#include <ctime>

void usage(const std::string name)
{
    std::cerr << "Usage\n";
    std::cerr << name << " INPUT [OPTIONS ...]\n";
    std::cerr << "  INPUT can be either a mp4 file name, a movie URL or a 'pgm' pattern.'\n";

    std::cerr << "\n  Input options:\n";
    std::cerr << "    --from TIME                 : time offset to start extracting from\n";
    std::cerr << "    --duration TIME             : time duration of the extracted clip\n";
    std::cerr << "    --poster TIME               : frame to extract the poster from (by default 1/3 of duration)ß\n";
    std::cerr << "    --fps FPS                   : for 'pgm' pattern, specifies the framerate to be used\n";
    std::cerr << "    --audio FILE                : for 'pgm', specifices a separate u8 22200 Hz wav file with audio\n";
    std::cerr << "    --srt FILE                  : burns the subtitle file into the flim\n";

    std::cerr << "\n  Output options:\n";
    std::cerr << "    --flim FILE                 : name of the flim file to create (by default 'out.flim')\n";
    std::cerr << "    --mp4 FILE                  : outputs a 60fps mp4 file with the result\n";
    std::cerr << "    --gif FILE                  : outputs a 20fps gif file with the result\n";
    std::cerr << "    --pgm PATTERN               : output every generated image in a pgm file\n";

    std::cerr << "\n  Encoding options:\n";
    std::cerr << "    --profile PROFILE           : presents the specific encoding profile, which sets a suitable default for all encoding options\n";
    std::cerr << "      Default is 'se30'. See below for description of profiles.\n";
    std::cerr << "    --silent BOOLEAN            : set to true for silent flims\n";
    std::cerr << "    --byterate BYTERATE         : bytes per ticks available for video compression\n";
    std::cerr << "    --fps-ratio BOOLEAN         : ratio of images from the source to drop.\n";
    std::cerr << "    --group BOOLEAN             : if true, packs ticks together to present screen updates at the same rate as the input media. Only works on a se30.\n";
    std::cerr << "    --bars BOOLEAN              : if false, image is zoomed in so there are no black bars.\n";
    std::cerr << "    --dither DITHER             : specifies the type of dithering to be used.\n";
    std::cerr << "      'ordered' will use a 4x4 ordered dither matrix.\n";
    std::cerr << "      'error' will use an error diffusion algorithm.\n";
    std::cerr << "    --error-algorithm ALGORITHM : error diffusion algorithm to be used\n";
    std::cerr << "      Default 'floyd'. See below for the list of valid error dithering algorithms.\n";
    std::cerr << "    --error-stability FLOAT     : amount of error to be accumulated before changing a screen pixel\n";
    std::cerr << "    --error-bidi BOOLEAN        : if true, error diffusion is applied in different direction for even and odd scanlines.\n";
    std::cerr << "    --error-bleed PERCENT       : how much error is moved from a pixel to the neighbours.\n";
    std::cerr << "    --filters FILTERS           : specifies a set of filters to be applied on image afgter resizing, but before dithering\n";
    std::cerr << "    --codec CODEC               : adds a specific codec to the encoding. The first --codec parameter clears the profile codec list\n";

    std::cerr << "\n  Misc options:\n";
    std::cerr << "    --watermark STRING          : adds the string to the upper left corner of the generated flim for identification purposes.\n";
    std::cerr << "      use 'auto' to use the encoding parameters as watermark\n";
    std::cerr << "    --debug BOOLEAN             : enables various debug options\n";

    std::cerr << "\nList of profiles names for the --profile option (default 'se30'):\n";
    for (auto n : { "128k", "512k", "xl", "plus", "se", "portable", "se30", "perfect" })
    {
        encoding_profile p;
        encoding_profile::profile_named(n, 512, 342, p);
        std::cerr << "        " << n << " : " << p.description() << "\n";
    }

    std::cerr << "\nList of error diffusion algorithms for the --error_diffusion option (default 'floyd'):\n";

    error_diffusion_algorithms([](const std::string name, const std::string description)
    {
        fprintf(stderr, "               %16s : %s\n", name.c_str(), description.c_str());
    });

    std::cerr << "use '" << name << " --help' for displaying this help page.\n";
}

#ifndef _WIN32
void segfault_handler(int signal)
{
    void *array[10];
    size_t size;

    // get void*'s for all entries on the stack
    size = backtrace(array, 10);

    // print out all the frames to stderr
    fprintf(stderr, "Error: signal %d:\n", signal);
    backtrace_symbols_fd(array, size, STDERR_FILENO);
    exit(1);
}
#endif

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

const std::string temp_file()
{
    std::string cache_file;
    #ifdef _WIN32
    char temp_path[MAX_PATH];
    if (GetTempPath(MAX_PATH, temp_path) == 0)
        throw "Failed to get temporary path\n";
    char temp_file[MAX_PATH];
    if (GetTempFileName(temp_path, "flim", 0, temp_file) == 0)
        throw "Failed to create temporary file\n";
    cache_file = temp_file;
#else
    char cache_file_template[] = "/tmp/flimmaker_cache_XXXXXX";
    int cache_fd = mkstemp(cache_file_template);
    if (cache_fd == -1)
        throw "Failed to create temporary file\n";
    cache_file = cache_file_template;
    close(cache_fd);
#endif
    return cache_file;
}

// The main function, does all the work
// flimmaker [-g] --in <%d.pgm> --from <index> --to <index> --cover <index> --audio <audio.wav> --flim <file>
int main(int argc, char **argv)
{
#ifndef _WIN32
    signal(SIGSEGV, segfault_handler);
#endif
    try
    {
        std::string input_file = "";
        std::string srt_file = "";
        std::string mp4_file = "";
        std::string gif_file = "";
        std::string out_arg = "out.flim";
        std::string audio_arg = "audio.raw";
        timestamp_t from_index = 0;  // #### This is not an index, it is a timestamp
        timestamp_t to_index = std::numeric_limits<double>::max();
        double duration = 300;  // 5 minutes by default
        timestamp_t poster_ts = -1;
        int cover_from = -1;
        int cover_to = -1;
        double fps = 24.0;
        std::string watermark = "";
        std::string pgm_pattern = ""; // "out-%06d.pgm";
        std::string diff_pattern = "";
        std::string change_pattern = "";
        std::string target_pattern = "";
        bool auto_watermark = false;
        std::string cache_file = temp_file();

        bool generated_cache = true;
        bool downloaded_file = false;
        bool profile_set = false;

        const std::string cmd_name{ argv[0] };

        // test_ffmpeg(argv[1]);

        std::vector<flimcompressor::codec_spec> codecs;
        codecs.push_back({});
        codecs.back().signature = 0x00;
        codecs.back().penality = 1;
        codecs.back().coder = std::make_shared<null_compressor>(0, 0);

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

        size_t width = 512;
        size_t height = 342;
        std::string profile_name = "se30";

        encoding_profile custom_profile;
        if (!encoding_profile::profile_named(profile_name, width, height, custom_profile)) {
            std::cerr << "Cannot find default profile '" << profile_name << "'\n";
            ::exit(EXIT_FAILURE);
        }

        test_simplesprintf();

        packz32opt_test();
        test_seconds_from_string();

        argc--;
        argv++;

        while (argc)
        {
            if (!strcmp(*argv, "--help"))
            {
                usage(cmd_name);
                ::exit(EXIT_SUCCESS);
            }

            if (strncmp(*argv, "--", 2))
            {
                if (input_file != "")
                {
                    std::cerr << "Input file specified twice: '" << input_file << "' and '" << *argv << "'\n";
                    ::exit(EXIT_FAILURE);
                }
                input_file = *argv;
            } else if (!strcmp(*argv, "--cache"))
            {
                argc--;
                argv++;
                cache_file = *argv;
                generated_cache = false;
            } else if (!strcmp(*argv, "--mp4"))
            {
                argc--;
                argv++;
                mp4_file = *argv;
            } else if (!strcmp(*argv, "--srt"))
            {
                argc--;
                argv++;
                srt_file = *argv;
            } else if (!strcmp(*argv, "--gif"))
            {
                argc--;
                argv++;
                gif_file = *argv;
            } else if (!strcmp(*argv, "--profile"))
            {
                argc--;
                argv++;
                profile_name = *argv;
                if (profile_name == "xl")
                {
                    width = (720 / 32) * 32;
                    height = 364;
                    std::cerr << "xl -- setting resolution to " << width << "x" << height << "\n";
                }
                if (profile_name == "portable")
                {
                    width = (640 / 32) * 32;
                    height = 400;
                    std::cerr << "portable -- setting resolution to " << width << "x" << height << "\n";
                }
                if (!encoding_profile::profile_named(profile_name, width, height, custom_profile))
                {
                    std::cerr << "Cannot find encoding profile '" << *argv << "'\n";
                    ::exit(EXIT_FAILURE);
                }
                profile_set = true;
            } else if (!strcmp(*argv, "--width"))
            {
                argc--;
                argv++;
                if (profile_set)
                {
                    std::cerr << "Changing width will reset setting profile to '" << profile_name << "'\n";
                }
                width = atoi(*argv);
                if ((width % 32) != 0)
                {
                    width = (width / 32) * 32;
                    std::cerr << "Width must be multiple of 32, rounding it down to '" << width << "'\n";
                }
                encoding_profile::profile_named(profile_name, width, height, custom_profile);
            } else if (!strcmp(*argv, "--height"))
            {
                argc--;
                argv++;
                if (profile_set)
                {
                    std::cerr << "Changing height will reset setting profile to '" << profile_name << "'\n";
                }
                height = atoi(*argv);
                encoding_profile::profile_named(profile_name, width, height, custom_profile);
            } else if (!strcmp(*argv, "--byterate"))
            {
                argc--;
                argv++;
                custom_profile.set_byterate(atoi(*argv));
            } else if (!strcmp(*argv, "--fps"))
            {
                argc--;
                argv++;
                fps = atof(*argv);
            } else if (!strcmp(*argv, "--fps-ratio"))
            {
                argc--;
                argv++;
                custom_profile.set_fps_ratio(atoi(*argv));
            } else if (!strcmp(*argv, "--group"))
            {
                argc--;
                argv++;
                custom_profile.set_group(bool_from(*argv));
            } else if (!strcmp(*argv, "--debug"))
            {
                argc--;
                argv++;
                sDebug = bool_from(*argv);
            } else if (!strcmp(*argv, "--from"))
            {
                argc--;
                argv++;
                from_index = seconds_from_string(*argv);
            } else if (!strcmp(*argv, "--to"))
            {
                argc--;
                argv++;
                to_index = atof(*argv);
            } else if (!strcmp(*argv, "--duration"))
            {
                argc--;
                argv++;
                duration = seconds_from_string(*argv);
            } else if (!strcmp(*argv, "--cover-from"))
            {
                argc--;
                argv++;
                cover_from = atoi(*argv);
            } else if (!strcmp(*argv, "--cover-to"))
            {
                argc--;
                argv++;
                cover_to = atoi(*argv);
            } else if (!strcmp(*argv, "--cover"))
            {
                argc--;
                argv++;
                cover_from = atoi(*argv);
                cover_to = cover_from + 23;
            } else if (!strcmp(*argv, "--poster"))
            {
                argc--;
                argv++;
                poster_ts = seconds_from_string(*argv);
            } else if (!strcmp(*argv, "--audio"))
            {
                argc--;
                argv++;
                audio_arg = *argv;
            } else if (!strcmp(*argv, "--flim"))
            {
                argc--;
                argv++;
                out_arg = *argv;
            } else if (!strcmp(*argv, "--out-pattern") || !strcmp(*argv, "--pgm-pattern") || !strcmp(*argv, "--pgm"))
            {
                argc--;
                argv++;
                pgm_pattern = *argv;
            } else if (!strcmp(*argv, "--diff-pattern"))
            {
                argc--;
                argv++;
                diff_pattern = *argv;
            } else if (!strcmp(*argv, "--change-pattern"))
            {
                argc--;
                argv++;
                change_pattern = *argv;
            } else if (!strcmp(*argv, "--target-pattern"))
            {
                argc--;
                argv++;
                target_pattern = *argv;
            } else if (!strcmp(*argv, "--comment"))
            {
                argc--;
                argv++;
                comment += "comment: ";
                comment += *argv;
                comment += "\n";
            } else if (!strcmp(*argv, "--watermark"))
            {
                argc--;
                argv++;
                if (!strcmp(*argv, "auto"))
                    auto_watermark = true;
                else
                    watermark = *argv;
            } else if (!strcmp(*argv, "--filters"))
            {
                argc--;
                argv++;
                custom_profile.set_filters(*argv);
            } else if (!strcmp(*argv, "--bars"))
            {
                argc--;
                argv++;
                custom_profile.set_bars(bool_from(*argv));
            } else if (!strcmp(*argv, "--codec"))
            {
                argc--;
                argv++;
                codecs.push_back(flimcompressor::make_codec(*argv, width, height));
            } else if (!strcmp(*argv, "--dither"))
            {
                argc--;
                argv++;
                custom_profile.set_dither(*argv);
            } else if (!strcmp(*argv, "--error-stability"))
            {
                argc--;
                argv++;
                custom_profile.set_stability(atof(*argv));
            } else if (!strcmp(*argv, "--error-algorithm"))
            {
                argc--;
                argv++;
                custom_profile.set_error_algorithm(*argv);
            } else if (!strcmp(*argv, "--error-bleed"))
            {
                argc--;
                argv++;
                custom_profile.set_error_bleed(atof(*argv));
            } else if (!strcmp(*argv, "--error-bidi"))
            {
                argc--;
                argv++;
                custom_profile.set_error_bidi(bool_from(*argv));
            } else if (!strcmp(*argv, "--silent"))
            {
                argc--;
                argv++;
                custom_profile.set_silent(bool_from(*argv));
            } else
            {
                std::cerr << "Unknown argument " << *argv << "\n";
                return EXIT_FAILURE;
            }

            argc--;
            argv++;
        }

        if (input_file == "")
        {
            usage(cmd_name);
            exit(EXIT_FAILURE);
        }

        std::vector<subtitle> subs;

        if (srt_file != "")
        {
            std::ifstream ifs;
            ifs.open(srt_file, std::ifstream::in);

            if (!ifs.good())
            {
                std::cerr << "ERROR: Cannot open subtitle file [" << srt_file << "]\n";
                exit(EXIT_FAILURE);
            }

            subs = ::read_subtitles(ifs);
            ifs.close();
            subs = ::subtitles_extract(subs, from_index, duration);
        }

        // If input-file is a URL, use yt-dlp to retrieve content
        if (input_file.rfind("https://", 0) == 0)
        {
            if (std::filesystem::exists(cache_file))
            {
                input_file = cache_file;
                std::clog << "Using cached file: '" << cache_file << "'\n";
            }
            else
            {
                char buffer[1024];
                auto input_url = input_file;

                sprintf(buffer, "yt-dlp '%s' -f mp4 --output '%s'", input_file.c_str(), cache_file.c_str());
                int res = system(buffer);
                if (res != 0)
                {
                    std::clog << "yt-dlp not installed or failing, falling back to youtube-dl (code " << res << ")\n";
                    sprintf(buffer, "youtube-dl '%s' -f mp4 --output '%s'", input_file.c_str(), cache_file.c_str());
                    res = system(buffer);
                    if (res != 0)
                    {
                        std::clog << "youtube-dl failed with error " << res << "\n";
                        exit(EXIT_FAILURE);
                    }
                }

                // Switch input file
                input_file = cache_file;
                downloaded_file = true;
            }
        }

        if (poster_ts == -1)
            poster_ts = duration / 3;

        if (codecs.size() > 1)
        {
            custom_profile.set_codecs(codecs);
        }

        if (auto_watermark)
        {
            if (watermark.size() > 0)
                watermark += " ";
            watermark += custom_profile.description();
        }

        std::clog << "Encoding arguments :\n" << custom_profile.description() << "\n";

        std::unique_ptr<input_reader> r;
        if (ends_with(input_file, ".pgm"))
        {
            std::clog << "Reading pgm from '" << input_file << "' pattern, at " << fps << " frames per second, using '" << audio_arg << "' audio file\n";
            std::clog << "( use --fps and --audio to change fps and audio )\n";
            r = make_filesystem_reader(input_file, fps, audio_arg, from_index, to_index);
        }
        else
        {
            r = make_ffmpeg_reader(input_file, from_index, duration);
            fps = r->frame_rate();
        }

        std::vector<std::unique_ptr<output_writer>> w;
        if (mp4_file != "")
            w.push_back(make_ffmpeg_writer(mp4_file, custom_profile.width(), custom_profile.height()));
        if (gif_file != "")
            w.push_back(make_gif_writer(gif_file, custom_profile.width(), custom_profile.height()));

        auto encoder = flimencoder{ custom_profile };
        encoder.set_fps(fps);
        encoder.set_comment(comment);
        encoder.set_cover(cover_from, cover_to + 1);
        encoder.set_watermark(watermark);
        encoder.set_out_pattern(pgm_pattern);
        encoder.set_diff_pattern(diff_pattern);
        encoder.set_change_pattern(change_pattern);
        encoder.set_target_pattern(target_pattern);
        encoder.set_poster_ts(poster_ts);
        encoder.set_subtitles(subs);

        encoder.make_flim(out_arg, r.get(), w);

        if (downloaded_file && generated_cache)
        {
            std::clog << "Removing '" << cache_file << "'\n";
            unlink(cache_file.c_str());
        }
    }
    catch (const char *error)
    {
        std::cerr << "**** ERROR: [" << error << "]\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}