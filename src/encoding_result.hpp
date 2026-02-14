#pragma once

#include <vector>
#include <cstdint>
#include <cstddef>
#include "codec_spec.hpp"
#include "bitmap.hpp"

namespace macflim
{

    class EncodingResult
    {
        const codec_spec &codec_;         //  Used codec
        bitmap image_;                    //  Resulting image
        const std::vector<uint8_t> data_; //  Resulting data
        const double quality_;            //  Resulting quality

    public:
        EncodingResult(
            const codec_spec &codec,
            const bitmap &current,
            const bitmap &target,
            const size_t budget) : codec_{codec},
                                   image_{current},
                                   data_{codec_.coder->compress(image_, target, budget * codec_.penality)},
                                   quality_{image_.proximity(target)}
        {
        }

        //  Encoded video with codec signature and trailer (#### why trailer?)
        std::vector<uint8_t> get_video_encoded_data() const
        {
            std::vector<uint8_t> result = {0x00, 0x00, 0x00, codec_.signature};
            result.insert(std::end(result), std::begin(data_), std::end(data_));
            return result;
        }

        double quality() const { return quality_; }
        const bitmap &image() const { return image_; }
    };

} // namespace macflim
