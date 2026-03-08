#pragma once

#include "bitmap.hpp"
#include "codec_spec.hpp"
#include "common.hpp"
#include "ditherer.hpp"
#include "encoding_result.hpp"
#include "frame.hpp"
#include "qhistogram.hpp"
#include "reader.hpp"
#include "subtitle_burner.hpp"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <format>
#include <iostream>
#include <vector>

namespace macflim
{

/// Manages frame compression workflow including dithering, codec selection, and budget allocation.
class compressor_helper
{
    ditherer &ditherer_;
    subtitle_burner &subtitle_burner_;
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
    compressor_helper(ditherer &d, subtitle_burner &subtitle_burner, const std::vector<codec_spec> &codecs,
                      const double fps, const size_t byterate, const std::vector<sound_frame_t> &audio,
                      const bool group)
        : ditherer_{d}, subtitle_burner_{subtitle_burner}, current_fb_{ditherer_.current()}, codecs_{codecs}, fps_{fps},
          byterate_{byterate}, audio_{audio}, group_{group}
    {
        current_tick_ = 0;
        in_fr_ = 0;
        current_audio_ = std::begin(audio_);
    }

    // Adds one grayscale to the generated video, keep track of previous
    // Returns the quality metric (proximity to target)
    double add(const grayscale &source);

    [[nodiscard]] std::vector<frame> get_frames() const;
};

} // namespace macflim
