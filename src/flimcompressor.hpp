#pragma once

#include <format>
#include <vector>
#include <functional>
#include <optional>
#include "errors.hpp"
#include "grayscale.hpp"
#include "frame.hpp"
#include "reader.hpp"
#include "codec_spec.hpp"
#include "dithering_parameters.hpp"
#include "ditherer.hpp"
#include "subtitle_burner.hpp"
#include "encoding_result.hpp"
#include "compressor_helper.hpp"
#include "profile.hpp"

using macflim::codec_spec;
using macflim::compressor_helper;
using macflim::Ditherer;
using macflim::dithering_parameters;
using macflim::make_codec;
using macflim::SubtitleBurner;

/**
 * The flimcompressor manages higher aspects of the compression
 */

class flimcompressor
{
private:
    size_t W_;
    size_t H_;

    std::function<std::optional<grayscale>()> next_image_;
    const std::vector<sound_frame_t> &audio_;
    const double fps_;
    std::vector<subtitle> subtitles_;

    std::vector<frame> frames_;
    std::optional<bitmap> initial_fb_;

public:
    flimcompressor(size_t W, size_t H, std::function<std::optional<grayscale>()> next_image, const std::vector<sound_frame_t> &audio, double fps, const std::vector<subtitle> &subtitles) : W_{W}, H_{H}, next_image_{std::move(next_image)}, audio_{audio}, fps_{fps}, subtitles_{subtitles} {}

    const std::vector<frame> &get_frames() const { return frames_; }
    const std::optional<bitmap> &get_initial() const { return initial_fb_; }

    void compress(const encoding_profile &profile, const std::string &watermark, initial_frame_mode initial_mode = initial_frame_mode::optional, bool loop = false);
};
