#pragma once

#include "reader.hpp"

std::unique_ptr<input_reader> make_filesystem_reader(
	std::string &input_file,
	double fps,
	std::string &audio_arg,
	double from_index,	//	bad name, is a timestamp
	double to_index
);
