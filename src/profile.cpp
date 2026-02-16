#include "profile.hpp"

namespace macflim
{

bool encoding_profile::set_dither(std::string dither)
{
    if (dither == "ordered")
        dither_ = grayscale::ordered;
    else if (dither == "error")
        dither_ = grayscale::error_diffusion;
    else if (dither == "blue")
        dither_ = grayscale::blue_noise;
    else
        throw macflim::config_error("Wrong dither option : only 'ordered', 'error', and 'blue' are supported", dither);
    return true;
}

bool encoding_profile::set_initial_mode(const std::string &mode)
{
    if (mode == "false" || mode == "none")
        initial_mode_ = initial_frame_mode::none;
    else if (mode == "optional")
        initial_mode_ = initial_frame_mode::optional;
    else if (mode == "true" || mode == "required")
        initial_mode_ = initial_frame_mode::required;
    else
        return false;
    return true;
}

std::vector<std::string> encoding_profile::parse_codec_array(const char *const *codec_array)
{
    std::vector<std::string> specs;
    for (size_t i = 0; i < MAX_CODECS; ++i)
    {
        if (codec_array[i] != nullptr)
            specs.push_back(codec_array[i]);
    }
    return specs;
}

bool encoding_profile::profile_named(const std::string name, encoding_profile &result)
{
    for (const auto &config : profile_table)
    {
        if (name == config.name)
        {
            result.set_size(config.width, config.height);
            result.set_byterate(config.byterate);
            result.set_filters(config.filters);
            result.set_fps_ratio(config.fps_ratio);
            result.set_group(config.group);
            result.set_stability(config.stability);
            result.set_bars(config.bars);
            result.set_dither(config.dither);
            result.set_error_algorithm(config.error_algorithm);
            result.set_error_bidi(config.error_bidi);
            result.set_error_bleed(config.error_bleed);
            result.set_codec_specs(parse_codec_array(config.codecs));
            result.set_silent(config.silent);
            return true;
        }
    }

    return false;
}

std::string encoding_profile::dither_string() const
{
    switch (dither_)
    {
    case grayscale::error_diffusion:
        return "error";
    case grayscale::ordered:
        return "ordered";
    case grayscale::blue_noise:
        return "blue";
    }
    return "???";
}

std::string encoding_profile::description() const
{
    std::ostringstream cmd;

    cmd << "--byterate " << byterate_;
    cmd << " --fps-ratio " << fps_ratio_;
    cmd << " --group " << (group_ ? "true" : "false");
    cmd << " --bars " << (bars_ ? "true" : "false");
    cmd << " --dither " << dither_string();
    if (dither_ == grayscale::error_diffusion)
    {
        cmd << " --error-stability " << stability_;
        cmd << " --error-algorithm " << error_algorithm_;
        cmd << " --error-bidi " << error_bidi_;
        cmd << " --error-bleed " << error_bleed_;
    }
    cmd << " --filters " << filters_;

    for (const auto &spec : codec_specs_)
        cmd << " --codec " << spec;

    cmd << " --silent " << (silent_ ? "true" : "false");

    // Add initial-frame mode to description
    const char *initial_mode_str = "optional";
    if (initial_mode_ == initial_frame_mode::none)
        initial_mode_str = "false";
    else if (initial_mode_ == initial_frame_mode::required)
        initial_mode_str = "true";
    cmd << " --initial-frame " << initial_mode_str;

    cmd << " --loop " << (loop_ ? "true" : "false");

    return cmd.str();
}

} // namespace macflim
