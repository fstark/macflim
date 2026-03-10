#pragma once

#include "common.hpp"
#include "compressor.hpp"
#include "errors.hpp"
#include "imgcompress.hpp"
#include "ruler.hpp"

#include <cstdint>
#include <format>
#include <memory>
#include <string>

namespace macflim
{

/// Configuration for a compression codec including its signature and performance penalty
/// Used to select and configure codecs for frame compression
struct codec_spec
{
    uint8_t signature;
    double penalty;
    std::shared_ptr<compressor> coder;
};

//  Codec factory functions
[[nodiscard]] static inline std::shared_ptr<compressor> make_z16_codec(size_t W, size_t H)
{
    return std::make_shared<vertical_compressor<uint16_t>>(W, H, uint16_ruler::ruler);
}

[[nodiscard]] static inline std::shared_ptr<compressor> make_z32_codec(size_t W, size_t H)
{
    return std::make_shared<vertical_compressor<uint32_t>>(W, H, uint32_ruler::ruler);
}

[[nodiscard]] static inline std::shared_ptr<compressor> make_invert_codec(size_t W, size_t H)
{
    return std::make_shared<invert_compressor>(W, H);
}

[[nodiscard]] static inline std::shared_ptr<compressor> make_lines_codec(size_t W, size_t H)
{
    return std::make_shared<copy_line_compressor>(W, H);
}

[[nodiscard]] static inline std::shared_ptr<compressor> make_null_codec(size_t W, size_t H)
{
    return std::make_shared<null_compressor>(W, H);
}

/// Codec registry entry mapping name to factory and metadata.
/// Used in codec_table to register available compression codecs.
struct codec_entry
{
    const char *name;
    uint8_t signature;
    double penalty;
    std::shared_ptr<compressor> (*factory)(size_t, size_t);
};

static constexpr codec_entry codec_table[] = {
    {"z16", 0x01, 0.45, make_z16_codec},       {"z32", 0x02, 1.00, make_z32_codec},
    {"invert", 0x03, 1.00, make_invert_codec}, {"lines", 0x04, 1.00, make_lines_codec},
    {"null", 0x00, 1.00, make_null_codec},
};

[[nodiscard]] inline codec_spec make_codec(std::string_view spec_string, size_t W, size_t H)
{
    auto spec_array = split(spec_string, ":");
    auto name = spec_array[0];
    std::string parameters_string = "";

    if (spec_array.size() > 1)
        parameters_string = spec_array[1];

    for (const auto &entry : codec_table)
    {
        if (name == entry.name)
        {
            codec_spec spec;
            spec.signature = entry.signature;
            spec.penalty = entry.penalty;
            spec.coder = entry.factory(W, H);

            for (auto &param_string : split(parameters_string, ","))
            {
                auto v = split(param_string, "=");
                std::string pname = v[0];
                std::string pvalue = "";
                if (v.size() > 1)
                    pvalue = v[1];
                spec.coder->set_parameter(pname, pvalue);
            }

            return spec;
        }
    }

    throw config_error("Unknown codec", name);
}

} // namespace macflim
