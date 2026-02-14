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

using macflim::codec_spec;
using macflim::compressor_helper;
using macflim::Ditherer;
using macflim::dithering_parameters;
using macflim::make_codec;
using macflim::SubtitleBurner;

class encoding_profile;

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

    void compress(const encoding_profile &profile, const std::string &watermark, initial_frame_mode initial_mode = initial_frame_mode::optional, bool loop = false)
    {
        // Parse codec specs into codec objects
        std::vector<codec_spec> codecs;
        for (const auto &spec_str : profile.codec_specs())
        {
            codecs.push_back(make_codec(spec_str, W_, H_));
        }

        grayscale previous(W_, H_);
        fill(previous, 0);

        // Pull all images from the callback
        // We need the first grayscale for initial frame generation, so pull it now
        auto first_opt = next_image_();
        if (!first_opt)
        {
            std::clog << std::format("Warning: no input images\n");
            return;
        }

        // Create dithering parameters from profile
        dithering_parameters dp = dithering_parameters::from_profile(profile, watermark);

        bool process_first_image = true;

        // Generate initial frame if requested or if looping is enabled
        if (loop || initial_mode != initial_frame_mode::none)
        {
            // Create temporary Ditherer to generate initial frame
            Ditherer temp_d{previous, dp};
            temp_d.dither(*first_opt);
            initial_fb_ = bitmap{temp_d.current()};

            // For 'required' mode, start encoding from this image
            // For 'optional' mode, keep previous as black (backwards compatible)
            if (initial_mode == initial_frame_mode::required)
            {
                copy(previous, temp_d.current());
                process_first_image = false;
            }
        }

        // Create dithering infrastructure for encoding
        Ditherer d{previous, dp};
        SubtitleBurner sb{subtitles_};
        compressor_helper ch{d, sb, codecs, fps_, profile.byterate(), audio_, profile.group()};

        // Process first image if not already used for initial frame
        if (process_first_image)
            ch.add(*first_opt);

        // Process remaining images from callback
        while (auto img = next_image_())
            ch.add(*img);

        // Perfect looping: add trailing frames until we return to initial frame
        if (loop && initial_fb_)
        {
            int trailing_count = 0;
            const int max_trailing = 100;
            double quality = 0.0;
            while (trailing_count < max_trailing && (quality = ch.add(*first_opt)) < 1.0)
            {
                trailing_count++;
            }
            if (quality < 1.0)
                std::clog << std::format("Warning: Loop did not achieve perfect quality after {} trailing frames (quality: {})\n",
                                         max_trailing, quality);
            else
                std::clog << std::format("Added {} trailing frames for perfect loop\n", trailing_count);
        }

        frames_ = ch.get_frames();
    }
};

#include "profile.hpp"
