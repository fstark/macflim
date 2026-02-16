#pragma once

#include "grayscale.hpp"

#include <cstdint>
#include <memory>
#include <array>

#include "reader.hpp"

namespace macflim
{

    class output_writer
    {

    public:
        virtual ~output_writer() {}

        virtual void write_frame(const grayscale &img, const sound_frame_t &snd) = 0;
    };

    std::unique_ptr<output_writer> make_ffmpeg_writer(const std::string &movie_path, size_t w, size_t h);
    std::unique_ptr<output_writer> make_gif_writer(const std::string &movie_path, size_t w, size_t h);
    std::unique_ptr<output_writer> make_pgm_writer(const std::string &pattern);
    std::unique_ptr<output_writer> make_null_writer();

} // namespace macflim
