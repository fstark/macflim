#pragma once

#include "image.hpp"

#include <cstdint>
#include <memory>
#include <array>

#include "reader.hpp"

class output_writer
{

public:
    virtual ~output_writer() {}

    virtual void write_frame( const image& img, const sound_frame_t &snd ) = 0;
};

std::unique_ptr<output_writer> make_ffmpeg_writer( const std::string &movie_path, size_t w, size_t h );
std::unique_ptr<output_writer> make_gif_writer( const std::string &movie_path, size_t w, size_t h );
std::unique_ptr<output_writer> make_null_writer();
