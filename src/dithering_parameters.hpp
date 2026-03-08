#pragma once

#include "grayscale.hpp"

#include <string>

namespace macflim
{

// Forward declaration
class encoding_profile;

struct dithering_parameters
{
    const bool bars;                   //  Do we add bars when we resize the added image?
    const std::string filters;         //  Filters to apply
    const double anchor_x;             //  Horizontal anchor for grayscale extraction
    const double anchor_y;             //  Vertical anchor for grayscale extraction
    const grayscale::dithering dither; //  The kind of dither to apply
    const std::string error_algorithm; //  Error algo
    const double stability;            //  Stability of the transform
    const float error_bleed;
    const bool error_bidi;
    const std::string watermark;

    // Factory method to construct from encoding profile
    static dithering_parameters from_profile(const encoding_profile &profile, const std::string &watermark);
};

} // namespace macflim
