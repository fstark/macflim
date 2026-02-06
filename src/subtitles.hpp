#pragma once

#include <vector>
#include <string>
#include <optional>
#include <tuple>
#include <iostream>

#include "common.hpp"

//  A subtitle
struct subtitle
{
    timestamp_t start;
    timestamp_t stop;
    std::vector<std::string> text = {};
    bool reverse = false;
};

//  Parse srt timestamp string
std::optional<std::tuple<timestamp_t, timestamp_t>> read_timestamps(const std::string &timestamps);

//  Parse srt file
std::optional<subtitle> next_subtitle(std::istream &in);

std::vector<subtitle> read_subtitles(std::istream &in);

/// Offset the subtitles of -from seconds and truncate so it is not longer than duration seconds.
std::vector<subtitle> subtitles_extract(const std::vector<subtitle> &subtitles, timestamp_t from, timestamp_t duration);
