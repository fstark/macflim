#pragma once

#include <cstddef>
#include "grayscale.hpp"
#include "dithering_parameters.hpp"
#include "watermark.hpp"
#include "errors.hpp"

namespace macflim
{

    /// This will dither a series of images, using the previous ones to minimize artifacts
    /// Size of the output is the same as the size of the initial image
    class Ditherer
    {
        size_t W_, H_;             //  Width and height of the generated image
        grayscale dithered_image_; //  The currently dithered image
                                   //  The initial grayscale defines the size of all future images

        const dithering_parameters dp_;

    public:
        Ditherer(const grayscale &initial_image, const dithering_parameters &dp) : W_{initial_image.W()},
                                                                                   H_{initial_image.H()},
                                                                                   dithered_image_{initial_image},
                                                                                   dp_{dp}
        {
        }

        size_t W() const { return W_; }
        size_t H() const { return H_; }

        /// Dither the grayscale according to the parameters
        void dither(const grayscale &img)
        {
            grayscale resized_image(W_, H_); //  note: was 512x342
            copy(resized_image, img, dp_.bars_, dp_.anchor_x_, dp_.anchor_y_);

            //  We filter the grayscale of the "right size", for things like corners, etc...
            grayscale filtered_image = filter(resized_image, dp_.filters_.c_str());

            grayscale dithered_image(W_, H_); //  The next dithered image

            if (dp_.dither_ == grayscale::error_diffusion)
                error_diffusion(dithered_image, filtered_image, dithered_image_, dp_.stability_, *get_error_diffusion_by_name(dp_.error_algorithm_), dp_.error_bleed_, dp_.error_bidi_);
            else if (dp_.dither_ == grayscale::ordered)
                ordered_dither(dithered_image, filtered_image, dithered_image_);
            else if (dp_.dither_ == grayscale::blue_noise)
                blue_noise_dither(dithered_image, filtered_image, dithered_image_);
            else
                throw config_error("Unknown dithering option", std::to_string(dp_.dither_));

            ::watermark(dithered_image, dp_.watermark_);

            //  The new dithered grayscale is the previous one
            dithered_image_ = dithered_image;
        }

        //  The current dithered image
        const grayscale current()
        {
            return dithered_image_;
        }
    };

} // namespace macflim
