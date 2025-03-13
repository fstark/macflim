#pragma once

#include "reader.hpp"

std::unique_ptr<input_reader> make_filesystem_reader(
	std::string &input_file,
	double fps,
	std::string &audio_arg,
	size_t from_index,
	size_t to_index
);
