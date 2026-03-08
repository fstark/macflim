#include "flimcompressor.hpp"
#include "profile.hpp"

namespace macflim
{

void flimcompressor::compress(const encoding_profile &profile, const std::string &watermark,
                              initial_frame_mode initial_mode, bool loop)
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
        // Create temporary ditherer to generate initial frame
        ditherer temp_d{previous, dp};
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
    ditherer d{previous, dp};
    subtitle_burner sb{subtitles_};
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
            std::clog << std::format(
                "Warning: Loop did not achieve perfect quality after {} trailing frames (quality: {})\n", max_trailing,
                quality);
        else
            std::clog << std::format("Added {} trailing frames for perfect loop\n", trailing_count);
    }

    frames_ = ch.get_frames();
}
} // namespace macflim