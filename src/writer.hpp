#pragma once

#include "grayscale.hpp"
#include "reader.hpp"

#include <array>
#include <cstdint>
#include <memory>

namespace macflim
{

/// Abstract interface for writing encoded frames to various output formats.
class output_writer
{

  public:
    virtual ~output_writer() {}

    virtual void write_frame(const grayscale &img, const sound_frame_t &snd) = 0;
};

[[nodiscard]] std::unique_ptr<output_writer> make_ffmpeg_writer(const std::string &movie_path, size_t w, size_t h);
[[nodiscard]] std::unique_ptr<output_writer> make_gif_writer(const std::string &movie_path, size_t w, size_t h);
[[nodiscard]] std::unique_ptr<output_writer> make_pgm_writer(const std::string &pattern);
[[nodiscard]] std::unique_ptr<output_writer> make_null_writer();

} // namespace macflim
