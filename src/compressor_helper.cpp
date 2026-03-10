#include "compressor_helper.hpp"

#include "encode_frame.hpp"

namespace macflim
{

// Gather audio samples for the given number of ticks
std::vector<uint8_t> compressor_helper::gather_audio(size_t local_ticks)
{
    std::vector<uint8_t> audio;
    for (size_t tick = 0; tick < local_ticks; tick++)
    {
        sound_frame_t snd;
        if (current_audio_ < std::end(audio_))
            snd = *current_audio_++;
        std::copy(snd.begin(), snd.end(), std::back_inserter(audio));
    }
    return audio;
}

// Log encoding progress to diagnostic output
void compressor_helper::log_encoding_progress() const
{
    if (!log_progress_)
        return;
    double time_s = current_tick_ / 60.0;
    int min = static_cast<int>(time_s / 60);
    double sec = time_s - min * 60;
    std::clog << std::format("Encoded frame {} ({}:{:05.2f}s)\r", frames_.size(), min, sec);
}

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

    size_t local_ticks = group_ ? ticks : 1;
    size_t num_subframes = ticks / local_ticks;
    assert(num_subframes * local_ticks == ticks);

    for (size_t subframe = 0; subframe < num_subframes; subframe++)
    {
        auto audio = gather_audio(local_ticks);
        //  Encode within budget, trying every codec, keeping the best
        auto best = encode_frame(current_fb_, fb, codecs_, byterate_ * local_ticks);

        //  Construct the frame with best video and audio
        frames_.push_back(frame{fb, local_ticks, best.get_video_encoded_data(), audio, best.image()});
        log_encoding_progress();
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
