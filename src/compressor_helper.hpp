#pragma once

#include <format>
#include <vector>
#include <algorithm>
#include <cstddef>
#include <iostream>
#include <cassert>
#include "ditherer.hpp"
#include "subtitle_burner.hpp"
#include "encoding_result.hpp"
#include "codec_spec.hpp"
#include "qhistogram.hpp"
#include "frame.hpp"
#include "bitmap.hpp"
#include "reader.hpp"
#include "common.hpp"

namespace macflim {

class CompressorHelper
{
    Ditherer &ditherer_;
    SubtitleBurner &subtitle_burner_;
    bitmap current_fb_; //  The bitmap displayed on screen at each step [#### check creation]
    const std::vector<codec_spec> &codecs_;
    const double fps_; //  Input fps
    const size_t byterate_;
    const std::vector<sound_frame_t> &audio_; //  The audio input
    bool group_;

    int in_fr_;                                                //  Input frame
    size_t current_tick_;                                      //  Output tick number
    std::vector<sound_frame_t>::const_iterator current_audio_; //  Current audio
    std::vector<frame> frames_;                                // Output generated frames
    bool log_progress_ = true;
    static const size_t BucketCount = 1000; //  Error distribution

    qhistogram<BucketCount> histo_;

public:
    CompressorHelper(
        Ditherer &ditherer,
        SubtitleBurner &subtitle_burner,
        const std::vector<codec_spec> &codecs,
        const double fps,
        const size_t byterate,
        const std::vector<sound_frame_t> &audio,
        const bool group) : ditherer_{ditherer},
                            subtitle_burner_{subtitle_burner},
                            current_fb_{ditherer_.current()},
                            codecs_{codecs},
                            fps_{fps},
                            byterate_{byterate},
                            audio_{audio},
                            group_{group}
    {
        current_tick_ = 0;
        in_fr_ = 0;
        current_audio_ = std::begin(audio_);
    }

    // Adds one grayscale to the generated video, keep track of previous
    // Returns the quality metric (proximity to target)
    double add(const grayscale &source)
    {
        //  Dither the new image
        ditherer_.dither(source);
        grayscale dest = ditherer_.current();
        subtitle_burner_.burn_into(dest, in_fr_ / fps_);

        //  True B&W packed image
        bitmap fb{dest};

        //  Let's see how many ticks we have to display this image
        in_fr_++;
        size_t next_tick = ticks_from_frame(in_fr_, fps_);
        size_t ticks = next_tick - current_tick_;
        assert(ticks > 0);

        size_t local_ticks = 1;

        if (group_)
            local_ticks = ticks;

        for (size_t i = 0; i != ticks; i += local_ticks)
        {
            //  Add as much audio as we have for the local ticks
            std::vector<uint8_t> audio;
            for (size_t i = 0; i != local_ticks; i++)
            {
                sound_frame_t snd;
                if (current_audio_ < std::end(audio_))
                    snd = *current_audio_++;
                std::copy(snd.begin(), snd.end(), std::back_inserter(audio));
            }

            //  Compute the video budget
            size_t video_budget = byterate_ * local_ticks;

            //  Encode within that budget with every codec
            std::vector<EncodingResult> encoding_results;
            std::transform(std::begin(codecs_), std::end(codecs_), std::back_inserter(encoding_results), [&](auto &codec) -> EncodingResult
                           { return EncodingResult(
                                 codec,
                                 current_fb_,
                                 fb,
                                 video_budget * codec.penality); });

            //  Find the result with highest quality
            auto best_result = std::max_element(encoding_results.begin(), encoding_results.end(), [](const EncodingResult &r1, const EncodingResult &r2)
                                                { return r1.quality() < r2.quality(); });

            //  Construct the frame with best video and audio
            frame f{fb, local_ticks, best_result->get_video_encoded_data(), audio, best_result->image()};

            frames_.push_back(f);
            if (log_progress_)
            {
                double time_s = current_tick_ / 60.0;
                int min = (int)(time_s / 60);
                double sec = time_s - min * 60;
                std::cerr << std::format("Encoded frame {} ({}:{:05.2f}s)\r", frames_.size(), min, sec);
            }

            current_fb_ = best_result->image();
        }

        auto q = frames_.back().result->proximity(fb);

        histo_.add(q);
        current_tick_ = next_tick;
        return q;
    }

    std::vector<frame> get_frames() const { return frames_; }
};

} // namespace macflim
