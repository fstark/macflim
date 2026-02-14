#pragma once

#include <string>
#include "grayscale.hpp"

namespace macflim {

struct DitheringParameters
{
    const bool bars_;                   //  Do we add bars when we resize the added image?  (note: maybe do some grayscale normalizer class that does all conversion work)
    const std::string filters_;         //  Filters to apply
    const double anchor_x_;             //  Horizontal anchor for grayscale extraction
    const double anchor_y_;             //  Vertical anchor for grayscale extraction
    const grayscale::dithering dither_; //  The kind of dither to apply
    const std::string error_algorithm_; //  Error algo
    const double stability_;            //  Stability of the transform
    const float error_bleed_;
    const bool error_bidi_;
    const std::string watermark_; //  Unsure if this should be here or higher
};

} // namespace macflim
