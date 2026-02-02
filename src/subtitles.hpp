#ifndef SUBTITLES_INCLUDED__
#define SUBTITLES_INCLUDED__

#include <vector>
#include <string>
#include <optional>
#include <tuple>
#include <iostream>

//  A subtltes 
struct subtitle
{
    double start;
    double stop;
    std::vector<std::string> text = {};
    bool reverse = false;
};


std::optional<std::tuple<double,double>> read_timestamps( const std::string &timestamps );
std::optional<subtitle> next_subtitle( std::istream &in );
std::vector<subtitle> read_subtitles( std::istream &in );
/// Offset the subtitles of -from seconds and truncate so it is not longer than duration seconds.
std::vector<subtitle> subtitles_extract( const std::vector<subtitle> &subtitles, double from, double duration );

#endif
