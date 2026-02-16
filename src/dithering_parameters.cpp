#include "dithering_parameters.hpp"
#include "profile.hpp"

namespace macflim
{

    dithering_parameters dithering_parameters::from_profile(const encoding_profile &profile, const std::string &watermark)
    {
        return dithering_parameters{
            profile.bars(),
            profile.filters(),
            profile.anchor_x(),
            profile.anchor_y(),
            profile.dither(),
            profile.error_algorithm(),
            profile.stability(),
            profile.error_bleed(),
            profile.error_bidi(),
            watermark};
    }

} // namespace macflim
