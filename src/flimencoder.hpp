#pragma once

#include <string>
#include <memory>
#include <vector>

#include "profile.hpp"
#include "subtitles.hpp"

#include "reader.hpp"
#include "writer.hpp"

extern bool sDebug;

class flimencoder
{
    const encoding_profile &profile_;

    std::unique_ptr<output_writer> pgm_poster_writer_;
    std::unique_ptr<output_writer> pgm_diff_writer_;
    std::unique_ptr<output_writer> pgm_change_writer_;
    std::unique_ptr<output_writer> pgm_target_writer_;

    std::vector<subtitle> subtitles_;

    double fps_ = 24;
    double poster_ts_ = 0;

    std::string comment_;

    std::string watermark_;

    size_t cover_begin_; /// Begin index of cover image
    size_t cover_end_;   /// End index of cover image

    int clamp(double v, int a, int b);
    std::vector<uint8_t> normalize_sound(std::vector<double> sound_samples, size_t len);

public:
    flimencoder(const encoding_profile &profile) : profile_{profile} {}

    void set_fps(double fps);
    void set_comment(const std::string comment);
    void set_cover(size_t cover_begin, size_t cover_end);
    void set_watermark(const std::string watermark);
    void set_pgm_poster_pattern(const std::string &pattern);
    void set_pgm_diff_pattern(const std::string &pattern);
    void set_pgm_change_pattern(const std::string &pattern);
    void set_pgm_target_pattern(const std::string &pattern);
    void set_poster_ts(double poster_ts);
    void set_subtitles(const std::vector<subtitle> &subtitles);

    void make_flim(const std::string flim_pathname, input_reader *reader, std::vector<sound_frame_t> audio_samples, const std::vector<std::unique_ptr<output_writer>> &writers);
};
