/**
 * The flimmaker tool takes a set of pgm images and generate a flim file suitable for playback by MacFlim on a vintage
 * Mac
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

#include "cmdline.hpp"
#include "errors.hpp"

#include <stdlib.h>
#ifndef _WIN32
#include <execinfo.h>
#include <signal.h>
#include <unistd.h>
#endif
#include <array>
#include <assert.h>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <limits>
#include <math.h>
#include <memory.h>
#include <memory>
#include <stdio.h>
#include <vector>
#define noLZG
#ifdef LZG
#include "lzg.h"
#endif
#include "common.hpp"
#include "ffmpeg_reader.hpp"
#include "filesystem_reader.hpp"
#include "flimencoder.hpp"
#include "flimutil.hpp"
#include "grayscale.hpp"
#include "imgcompress.hpp"
#include "reader.hpp"
#include "subtitles.hpp"
#include "writer.hpp"

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

using namespace std::string_literals;

// If defined, we add a "stamp" to each stream, to know where it is coming from
#define noSTAMP

#ifdef STAMP
static int sStream = 0;
#endif

#ifndef VERSION
#define VERSION "dev-unknown"
#endif

namespace macflim
{

// True if the global '-g' option was set
bool sDebug = false;

const char *version = VERSION;

// Write a bunch of bytes in a file
void write_data(const char *file, uint8_t *data, size_t len)
{
    FILE *f = fopen(file, "wb");
    while (len--)
        fputc(*data++, f);
    fclose(f);
}

#ifndef _WIN32
void segfault_handler(int signal)
{
    void *array[10];
    size_t size;

    // get void*'s for all entries on the stack
    size = backtrace(array, 10);

    // print out all the frames to stderr
    std::cerr << std::format("Error: signal {}:\n", signal);
    backtrace_symbols_fd(array, size, STDERR_FILENO);
    exit(1);
}
#endif

const std::string temp_file()
{
    std::string cache_file;
#ifdef _WIN32
    char temp_path[MAX_PATH];
    if (GetTempPath(MAX_PATH, temp_path) == 0)
        throw io_error("Failed to get temporary path", "<temp>");
    char temp_file[MAX_PATH];
    if (GetTempFileName(temp_path, "flim", 0, temp_file) == 0)
        throw io_error("Failed to create temporary file", temp_path);
    cache_file = temp_file;
#else
    char cache_file_template[] = "/tmp/flimmaker_cache_XXXXXX";
    int cache_fd = mkstemp(cache_file_template);
    if (cache_fd == -1)
        throw io_error("Failed to create temporary file", "/tmp");
    cache_file = cache_file_template;
    close(cache_fd);
#endif
    return cache_file;
}

// The main function, does all the work
// flimmaker [-g] --in <%d.pgm> --from <index> --to <index> --cover <index> --audio <audio.wav> --flim <file>
int run_main(int argc, char **argv)
{
#ifndef _WIN32
    signal(SIGSEGV, segfault_handler);
#endif
    try
    {
        test_simplesprintf();
        packz32opt_test();
        test_seconds_from_string();

        auto opts = parse_arguments(argc, argv);

        std::vector<subtitle> subs;

        if (opts.srt_file != "")
        {
            std::ifstream ifs;
            ifs.open(opts.srt_file, std::ifstream::in);

            if (!ifs.good())
            {
                std::cerr << std::format("ERROR: Cannot open subtitle file [{}]\n", opts.srt_file);
                exit(EXIT_FAILURE);
            }

            subs = read_subtitles(ifs);
            ifs.close();
            subs = subtitles_extract(subs, opts.from_index, opts.duration);
        }

        // If input-file is a URL, use yt-dlp to retrieve content
        if (opts.input_file.rfind("https://", 0) == 0)
        {
            if (std::filesystem::exists(opts.cache_file))
            {
                opts.input_file = opts.cache_file;
                std::clog << std::format("Using cached file: '{}'\n", opts.cache_file);
            }
            else
            {
                auto input_url = opts.input_file;

                std::string buffer = std::format("yt-dlp '{}' -f mp4 --output '{}'", opts.input_file, opts.cache_file);
                int res = system(buffer.c_str());
                if (res != 0)
                {
                    std::clog << std::format("yt-dlp not installed or failing, falling back to youtube-dl (code {})\n", res);
                    buffer = std::format("youtube-dl '{}' -f mp4 --output '{}'", opts.input_file, opts.cache_file);
                    res = system(buffer.c_str());
                    if (res != 0)
                    {
                        std::clog << std::format("youtube-dl failed with error {}\n", res);
                        exit(EXIT_FAILURE);
                    }
                }

                // Switch input file
                opts.input_file = opts.cache_file;
                opts.downloaded_file = true;
            }
        }

        if (opts.poster_ts == -1 && ends_with(opts.input_file, ".pgm"))
            opts.poster_ts = opts.duration / 3;

        if (opts.auto_watermark)
        {
            if (opts.watermark.size() > 0)
                opts.watermark += " ";
            opts.watermark += opts.custom_profile.description();
        }

        std::clog << std::format("Encoding arguments :\n{}\n", opts.custom_profile.description());

        std::unique_ptr<input_reader> r;
        if (ends_with(opts.input_file, ".pgm"))
        {
            std::clog << std::format("Reading pgm from '{}' pattern, at {} frames per second, using '{}' audio file\n",
                                     opts.input_file, opts.fps, opts.audio_file);
            std::clog << "( use --fps and --audio to change fps and audio )\n";
            r = make_filesystem_reader(opts.input_file, opts.fps, opts.audio_file, opts.from_index, opts.to_index);
        }
        else
        {
            r = make_ffmpeg_reader(opts.input_file, opts.from_index, opts.duration);
            opts.fps = r->frame_rate();
        }

        // Pass 1: decode audio separately (before video pass)
        std::vector<sound_frame_t> audio_samples;
        if (!opts.custom_profile.silent() && !ends_with(opts.input_file, ".pgm"))
        {
            audio_samples = decode_audio(opts.input_file, opts.from_index, opts.duration);
        }

        // Compute poster timestamp now that we know actual content duration
        if (opts.poster_ts == -1)
        {
            if (!audio_samples.empty())
                opts.poster_ts = (audio_samples.size() / 60.0) / 3; // 1/3 of actual audio duration
            else
                opts.poster_ts = opts.duration / 3;
        }

        std::vector<std::unique_ptr<output_writer>> w;
        if (opts.mp4_file != "")
            w.push_back(make_ffmpeg_writer(opts.mp4_file, opts.custom_profile.width(), opts.custom_profile.height()));
        if (opts.gif_file != "")
            w.push_back(make_gif_writer(opts.gif_file, opts.custom_profile.width(), opts.custom_profile.height()));
        if (opts.pgm_pattern != "")
            w.push_back(make_pgm_writer(opts.pgm_pattern));

        auto encoder = flimencoder(opts.custom_profile);
        encoder.set_fps(opts.fps);
        encoder.set_comment(opts.comment);
        encoder.set_cover(opts.cover_from, opts.cover_to + 1);
        encoder.set_watermark(opts.watermark);
        encoder.set_pgm_poster_pattern(opts.pgm_poster_pattern);
        encoder.set_pgm_diff_pattern(opts.diff_pattern);
        encoder.set_pgm_change_pattern(opts.change_pattern);
        encoder.set_pgm_target_pattern(opts.target_pattern);
        encoder.set_poster_ts(opts.poster_ts);
        encoder.set_subtitles(subs);

        encoder.make_flim(opts.flim_file, r.get(), std::move(audio_samples), w);

        if (opts.downloaded_file && opts.generated_cache)
        {
            std::clog << std::format("Removing '{}'\n", opts.cache_file);
            unlink(opts.cache_file.c_str());
        }
    }
    catch (const std::exception &error)
    {
        std::cerr << std::format("**** ERROR: [{}]\n", error.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

} // namespace macflim

int main(int argc, char **argv)
{
    return macflim::run_main(argc, argv);
}