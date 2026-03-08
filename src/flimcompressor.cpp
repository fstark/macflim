#include "flimcompressor.hpp"

#include "profile.hpp"

namespace macflim
{

void flimcompressor::generate_initial_frame(const grayscale &first_image, grayscale &previous,
                                            initial_frame_mode initial_mode, const dithering_parameters &dp,
                                            bool &process_first_image)
{
    ditherer temp_d{previous, dp};
    temp_d.dither(first_image);
    initial_fb_ = bitmap{temp_d.current()};

    // For 'required' mode, start encoding from this image
    // For 'optional' mode, keep previous as black (backwards compatible)
    if (initial_mode == initial_frame_mode::required)
    {
        copy(previous, temp_d.current());
        process_first_image = false;
    }
}

void flimcompressor::add_trailing_loop_frames(compressor_helper &ch, const grayscale &first_image)
{
    int trailing_count = 0;
    constexpr int max_trailing = 100;
    double quality = 0.0;
    while (trailing_count < max_trailing && (quality = ch.add(first_image)) < 1.0)
        trailing_count++;

    if (quality < 1.0)
        std::clog << std::format(
            "Warning: Loop did not achieve perfect quality after {} trailing frames (quality: {})\n", max_trailing,
            quality);
    else
        std::clog << std::format("Added {} trailing frames for perfect loop\n", trailing_count);
}

void flimcompressor::compress(const encoding_profile &profile, const std::string &watermark,
                              initial_frame_mode initial_mode, bool loop)
{
    // Parse codec specs into codec objects
    std::vector<codec_spec> codecs;
    for (const auto &spec_str : profile.codec_specs())
        codecs.push_back(make_codec(spec_str, W_, H_));

    grayscale previous(W_, H_);
    fill(previous, 0);

    // Pull first image (needed for initial frame generation)
    auto first_opt = next_image_();
    if (!first_opt)
    {
        std::clog << std::format("Warning: no input images\n");
        return;
    }

    dithering_parameters dp = dithering_parameters::from_profile(profile, watermark);
    bool process_first_image = true;

    if (loop || initial_mode != initial_frame_mode::none)
        generate_initial_frame(*first_opt, previous, initial_mode, dp, process_first_image);

    // Create dithering infrastructure for encoding
    ditherer d{previous, dp};
    subtitle_burner sb{subtitles_};
    compressor_helper ch{d, sb, codecs, fps_, profile.byterate(), audio_, profile.group()};

    if (process_first_image)
        ch.add(*first_opt);

    while (auto img = next_image_())
        ch.add(*img);

    if (loop && initial_fb_)
        add_trailing_loop_frames(ch, *first_opt);

    frames_ = ch.get_frames();
}
} // namespace macflim