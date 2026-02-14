#pragma once

#include <format>
#include <string>
#include <memory>
#include <cstdint>
#include "common.hpp"
#include "errors.hpp"
#include "compressor.hpp"
#include "imgcompress.hpp"
#include "ruler.hpp"

namespace macflim {

struct codec_spec
{
    uint8_t signature;
    double penality;
    std::shared_ptr<compressor> coder;
};

inline codec_spec make_codec(const std::string &spec_string, size_t W, size_t H)
{
    auto spec_array = split(spec_string, ":");
    auto name = spec_array[0];
    std::string parameters_string = "";

    if (spec_array.size() > 1)
        parameters_string = spec_array[1];

    codec_spec spec;
    spec.signature = 0x00;
    spec.penality = 1;
    spec.coder = std::make_shared<null_compressor>(W, H);

    if (name == "z16")
    {
        spec.signature = 0x01;
        spec.penality = 0.45;
        spec.coder = std::make_shared<vertical_compressor<uint16_t>>(W, H, uint16_ruler::ruler);
    }
    else if (name == "z32")
    {
        spec.signature = 0x02;
        spec.penality = 1.00;
        spec.coder = std::make_shared<vertical_compressor<uint32_t>>(W, H, uint32_ruler::ruler);
    }
    else if (name == "z32old")
    {
        static bit_ruler<uint32_t> br32;
        spec.signature = 0x02;
        spec.penality = 1.00;
        spec.coder = std::make_shared<vertical_compressor<uint32_t>>(W, H, br32);
    }
    else if (name == "invert")
    {
        spec.signature = 0x03;
        spec.penality = 1.00;
        spec.coder = std::make_shared<invert_compressor>(W, H);
    }
    else if (name == "lines")
    {
        spec.signature = 0x04;
        spec.penality = 1.00;
        spec.coder = std::make_shared<copy_line_compressor>(W, H);
    }
    else if (name == "null")
    {
        spec.signature = 0x00;
        spec.penality = 1.00;
        spec.coder = std::make_shared<null_compressor>(W, H);
    }
    else
    {
        std::clog << std::format("Unknown codec: [{}]\n", name);
        throw config_error("Unknown codec", name);
    }

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

} // namespace macflim
