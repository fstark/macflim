#include "compressor_helper.hpp"

namespace macflim
{

// Adds one grayscale to the generated video, keep track of previous
// Returns the quality metric (proximity to target)
double compressor_helper::add(const grayscale &source)
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

    size_t num_subframes = ticks / local_ticks;
    assert(num_subframes * local_ticks == ticks); // Verify no remainder

    for (size_t subframe = 0; subframe < num_subframes; subframe++)
    {
        //  Add as much audio as we have for the local ticks
        std::vector<uint8_t> audio;
        for (size_t tick = 0; tick < local_ticks; tick++)
        {
            sound_frame_t snd;
            if (current_audio_ < std::end(audio_))
                snd = *current_audio_++;
            std::copy(snd.begin(), snd.end(), std::back_inserter(audio));
        }

        //  Compute the video budget
        size_t video_budget = byterate_ * local_ticks;

        //  Encode within that budget with every codec
        std::vector<encoding_result> encoding_results;
        std::transform(std::begin(codecs_), std::end(codecs_), std::back_inserter(encoding_results),
                       [&](auto &codec) -> encoding_result
                       { return encoding_result(codec, current_fb_, fb, video_budget); });

        //  Find the result with highest quality
        auto best_result = std::max_element(encoding_results.begin(), encoding_results.end(),
                                            [](const encoding_result &r1, const encoding_result &r2)
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

std::vector<frame> compressor_helper::get_frames() const
{
    return frames_;
}

} // namespace macflim
