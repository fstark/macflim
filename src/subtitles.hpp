#pragma once

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




//  Parse srt timestamp string
std::optional<std::tuple<double,double>> read_timestamps( const std::string &timestamps )
{
    int h0,m0,s0,ms0;
    int h1,m1,s1,ms1;
    if (sscanf( timestamps.c_str(), "%d:%d:%d,%d --> %d:%d:%d,%d", &h0, &m0, &s0, &ms0, &h1, &m1, &s1, &ms1 )!=8)
        return std::nullopt;
    auto start = h0*3600+m0*60+s0+ms0/1000.0;
    auto stop = h1*3600+m1*60+s1+ms1/1000.0;
    return std::tuple<double,double>{ start, stop };
}

using namespace std::string_literals;

//  Parse srt file
std::optional<subtitle> next_subtitle( std::istream &in )
{
    std::vector<std::string> lines;
    std::string line;

        //  Index
    if (!std::getline(in,line))
        return std::nullopt;

    while (1)
    {
            //  Timestamps
        if (!std::getline(in,line))
            return std::nullopt;

        if (auto timestamps = read_timestamps( line ))
        {
            subtitle current{ std::get<0>(timestamps.value()), std::get<1>(timestamps.value()) };
            while (std::getline(in,line))
            {
                // std::cout << "-->" << line << "<--\n";
                while (line.size()>=1 && line.back()=='\r')
                    line.pop_back();

                if (line==""s)
                    break;

                if (line.front()=='[' && line.back()==']')
                {
                    line.pop_back();
                    line.erase( line.begin() );
                    current.reverse = true;
                }

                current.text.push_back( line );
            }

            return current;
        }

        //  If we failed the timestamp, we scan for the next timestamp
    }

    //  NOT REACHED
}

std::vector<subtitle> read_subtitles( std::istream &in )
{
    std::vector<subtitle> result;
    while (auto s=next_subtitle( in ))
    {
        result.push_back( s.value() );
    }

    return result;
}

/// Offset the subtitles of -from seconds and truncate so it is not longer than duration seconds.
std::vector<subtitle> subtitles_extract( const std::vector<subtitle> &subtitles, double from, double duration )
{
    std::vector<subtitle> result;

    for (auto &sub:subtitles)
    {
        auto sub2 =sub;
        sub2.start -= from;
        sub2.stop -= from;
        if (sub2.stop>0 && sub2.start<duration)
        {
            if (sub2.start<=0)
                sub2.start = 0;
            if (sub2.stop>duration)
                sub2.stop = duration;
            result.push_back( sub2 );
        }
    }

    return result;
}
