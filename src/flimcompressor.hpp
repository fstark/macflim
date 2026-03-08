#pragma once

#include "codec_spec.hpp"
#include "compressor_helper.hpp"
#include "ditherer.hpp"
#include "dithering_parameters.hpp"
#include "encoding_result.hpp"
#include "errors.hpp"
#include "frame.hpp"
#include "grayscale.hpp"
#include "profile.hpp"
#include "reader.hpp"
#include "subtitle_burner.hpp"

#include <format>
#include <functional>
#include <optional>
#include <vector>

namespace macflim
{

/**
 * The flimcompressor manages higher aspects of the compression
 */

/// Manages the complete compression pipeline for a sequence of images.
/// Coordinates dithering, codec selection, budgeting, and frame generation.
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
    flimcompressor(size_t W, size_t H, std::function<std::optional<grayscale>()> next_image,
                   const std::vector<sound_frame_t> &audio, double fps, const std::vector<subtitle> &subtitles)
        : W_{W}, H_{H}, next_image_{std::move(next_image)}, audio_{audio}, fps_{fps}, subtitles_{subtitles}
    {
    }

    [[nodiscard]] const std::vector<frame> &get_frames() const
    {
        return frames_;
    }
    [[nodiscard]] const std::optional<bitmap> &get_initial() const
    {
        return initial_fb_;
    }

    void compress(const encoding_profile &profile, const std::string &watermark,
                  initial_frame_mode initial_mode = initial_frame_mode::optional, bool loop = false);
};

} // namespace macflim
