#pragma once

#include <string>
#include "grayscale.hpp"

namespace macflim
{

    // Forward declaration
    class encoding_profile;

    struct dithering_parameters
    {
        const bool bars_;                   //  Do we add bars when we resize the added image?
        const std::string filters_;         //  Filters to apply
        const double anchor_x_;             //  Horizontal anchor for grayscale extraction
        const double anchor_y_;             //  Vertical anchor for grayscale extraction
        const grayscale::dithering dither_; //  The kind of dither to apply
        const std::string error_algorithm_; //  Error algo
        const double stability_;            //  Stability of the transform
        const float error_bleed_;
        const bool error_bidi_;
        const std::string watermark_;

        // Factory method to construct from encoding profile
        static dithering_parameters from_profile(const encoding_profile &profile, const std::string &watermark);
    };

} // namespace macflim
