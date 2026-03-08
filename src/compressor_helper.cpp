#include "compressor_helper.hpp"

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

// Encode with every codec and return the best result
encoding_result compressor_helper::encode_best(const bitmap &fb, size_t video_budget)
{
    std::vector<encoding_result> results;
    std::transform(std::begin(codecs_), std::end(codecs_), std::back_inserter(results),
                   [&](auto &codec) -> encoding_result
                   { return encoding_result(codec, current_fb_, fb, video_budget); });

    return *std::max_element(results.begin(), results.end(), [](const encoding_result &r1, const encoding_result &r2)
                             { return r1.quality() < r2.quality(); });
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
    ditherer_.dither(source);
    grayscale dest = ditherer_.current();
    subtitle_burner_.burn_into(dest, in_fr_ / fps_);

    bitmap fb{dest};

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
        auto best = encode_best(fb, byterate_ * local_ticks);

        frames_.push_back(frame{fb, local_ticks, best.get_video_encoded_data(), audio, best.image()});
        log_encoding_progress();
        current_fb_ = best.image();
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
