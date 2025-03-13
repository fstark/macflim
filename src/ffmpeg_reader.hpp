#pragma	once

#include "reader.hpp"

std::unique_ptr<input_reader> make_ffmpeg_reader(const std::string &movie_path, double from, double to);
