#pragma once

#include <string>
#include <memory>
#include <array>
#include <iostream>
#include <algorithm>
#include <cmath>

#include "common.hpp"

#include "image.hpp"

/// A macintosh formatted sound frame (370 bytes)
class sound_frame_t
{
public:
    static const size_t size = 370;

protected:
    std::array<uint8_t, size> data_;

public:
    sound_frame_t()
    {
        for (int i = 0; i != size; i++)
            data_[i] = 128;
    }

    uint8_t &at(size_t i) { return data_[i]; }

    std::array<uint8_t, size>::const_iterator begin() const { return std::cbegin(data_); }
    std::array<uint8_t, size>::const_iterator end() const { return std::cend(data_); }
};

/// Bundles together an image and all the sound frames that are played during the display of the image
/// A frame has an audio timestamp (the ts at which the audio starts)
//  and a video timestamp (the ts at which the image is displayed)
//  (hopefully the video timestamp occurs within the audio timestamp)
class frame_t
{
public:
    timestamp_t audio_ts = 0;
    timestamp_t video_ts = 0;
    std::vector<sound_frame_t> sounds;      //  A vector of 1/60th of a second sound frames
    std::unique_ptr<image> img;

    timestamp_t audio_end() const { return audio_ts + sounds.size() / 60.0; }

    //  Makes one frame from two consecutive frames
    //  First frame is deleted
    void append( frame_t &other )
    {
        assert( equals( audio_end(), other.audio_ts ) );
        sounds.insert(sounds.end(), other.sounds.begin(), other.sounds.end());
        img = std::move(other.img);
        // audio_ts does not move
        video_ts = other.video_ts;
    }
};

/// Accumulates audio and video
class frame_accumulator
{
    std::vector<sound_frame_t> audio_;
    std::vector<std::unique_ptr<image>> images_;
};


/*
#include <generator>

std::generator<unsigned int> f()
{
    unsigned int i = 1;
    while (1)
    {
        co_yield i;
        i *= 2;
    }
}
*/

//  Abstract class to read data from a source
//  Sources can be list of still images in the filesystem or a movie file
class input_reader
{
public:
    virtual ~input_reader() {}

    //  Frame rate of the returned images
    virtual double frame_rate() = 0;

    //  Return next image until no more images are available
    virtual std::unique_ptr<image> next() = 0;
    // virtual std::vector<image> images() = 0;

    //  Get the next sound sample, mac format
    virtual std::unique_ptr<sound_frame_t> next_sound() = 0;
};

inline size_t ticks_from_frame(size_t n, double fps) { return n / fps * 60 + .5; }
