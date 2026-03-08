#pragma once

#include "common.hpp"
#include "profile.hpp"

#include <limits>
#include <string>
#include <vector>

namespace macflim
{

extern bool sDebug;

struct program_options
{
    std::string input_file = "";
    std::string srt_file = "";
    std::string mp4_file = "";
    std::string gif_file = "";
    std::string flim_file = "out.flim";
    std::string audio_file = "audio.raw";
    std::string cache_file;
    bool generated_cache = true;
    bool downloaded_file = false;

    timestamp_t from_index = 0;
    timestamp_t to_index = std::numeric_limits<double>::max();
    double duration = 4 * 60 * 60;
    timestamp_t poster_ts = -1;
    int cover_from = -1;
    int cover_to = -1;
    double fps = 24.0;

    std::string watermark = "";
    bool auto_watermark = false;
    std::string pgm_pattern = "";
    std::string pgm_poster_pattern = "";
    std::string diff_pattern = "";
    std::string change_pattern = "";
    std::string target_pattern = "";

    size_t width = 0;
    size_t height = 0;
    std::string profile_name = "se30";
    encoding_profile custom_profile;
    std::vector<std::string> user_codec_specs;

    std::string comment;
};

void usage(const std::string name);

program_options parse_arguments(int argc, char **argv);

} // namespace macflim
