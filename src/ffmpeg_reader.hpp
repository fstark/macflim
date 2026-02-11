#pragma once

#include "reader.hpp"

std::unique_ptr<input_reader> make_ffmpeg_reader(const std::string &movie_path, timestamp_t from, timestamp_t duration);

/// Decode audio from a media file and return pre-converted Mac sound frames.
/// Opens the file, decodes only audio packets, normalizes, and converts to 370-byte frames.
/// Returns an empty vector if the file has no audio stream.
std::vector<sound_frame_t> decode_audio(const std::string &movie_path, timestamp_t from, timestamp_t duration);
