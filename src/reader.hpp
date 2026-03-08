#pragma once

#include "common.hpp"
#include "constants.hpp"
#include "grayscale.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>

namespace macflim
{

/// A Macintosh formatted sound frame (370 bytes).
/// Wraps a fixed-size audio buffer for one tick (1/60th second) of audio playback.
class sound_frame_t
{
  public:
    static constexpr size_t size = constants::sound_frame_bytes;

  protected:
    std::array<uint8_t, size> data_;

  public:
    sound_frame_t()
    {
        for (int i = 0; i != size; i++)
            data_[i] = 128;
    }

    uint8_t &at(size_t i)
    {
        return data_[i];
    }

    std::array<uint8_t, size>::const_iterator begin() const
    {
        return std::cbegin(data_);
    }
    std::array<uint8_t, size>::const_iterator end() const
    {
        return std::cend(data_);
    }
};

/// Bundles together an grayscale and all the sound frames that are played during the display of the image
/// A frame has an audio timestamp (the ts at which the audio starts)
//  and a video timestamp (the ts at which the grayscale is displayed)
//  (hopefully the video timestamp occurs within the audio timestamp)
struct frame_t
{
    timestamp_t audio_ts = 0;
    timestamp_t video_ts = 0;
    std::vector<sound_frame_t> sounds; //  A vector of 1/60th of a second sound frames
    std::unique_ptr<grayscale> img;

    timestamp_t audio_end() const
    {
        return audio_ts + sounds.size() / 60.0;
    }

    //  Makes one frame from two consecutive frames
    //  First frame is deleted
    void append(frame_t &other)
    {
        assert(equals(audio_end(), other.audio_ts));
        sounds.insert(sounds.end(), other.sounds.begin(), other.sounds.end());
        img = std::move(other.img);
        // audio_ts does not move
        video_ts = other.video_ts;
    }
};

//  Abstract class to read data from a source
//  Sources can be list of still images in the filesystem or a movie file
class input_reader
{
  public:
    virtual ~input_reader() {}

    //  Frame rate of the returned images
    [[nodiscard]] virtual double frame_rate() = 0;

    //  Return next grayscale until no more images are available
    [[nodiscard]] virtual std::unique_ptr<grayscale> next() = 0;

    //  Get the next sound sample, mac format
    [[nodiscard]] virtual std::unique_ptr<sound_frame_t> next_sound() = 0;
};

} // namespace macflim
