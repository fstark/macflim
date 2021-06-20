#ifndef WRITER_INCLUDED__
#define WRITER_INCLUDED__

#include "image.hpp"

#include <cstdint>
#include <memory>
#include <array>

class output_writer
{

public:
    virtual ~output_writer() {}

    typedef std::array<uint8_t,370> sound_frame_t;

    virtual void write_frame( const image& img, const sound_frame_t &snd ) = 0;
};

std::unique_ptr<output_writer> make_ffmpeg_writer( const std::string &movie_path, size_t w, size_t h );
std::unique_ptr<output_writer> make_null_writer();

#endif

