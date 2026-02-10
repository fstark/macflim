#pragma once

//  Shared flim format types used by both the encoder and the utility

#include "imgcompress.hpp"

#include <cstdint>
#include <cstddef>
#include <vector>
#include <string>

//  Component types in a flim file
enum eComponentType
{
    component_info = 0,
    component_movie = 1,
    component_toc = 2,
    component_poster = 3,
    component_initial = 4
};

inline const char *component_type_name( uint16_t type )
{
    switch (type)
    {
        case component_info:    return "info";
        case component_movie:   return "movie";
        case component_toc:     return "toc";
        case component_poster:  return "poster";
        case component_initial: return "initial";
        default:                return "unknown";
    }
}

//  The info component of a flim, as stored on disk
struct flim_info
{
    size_t width_;          //  2 bytes
    size_t height_;         //  2 bytes
    bool silent_;           //  2 bytes
    size_t frame_count_;    //  4 bytes
    size_t total_ticks_;    //  4 bytes
    size_t byterate_;       //  2 bytes

    flim_info() : width_(0), height_(0), silent_(false),
                  frame_count_(0), total_ticks_(0), byterate_(0)
    {
    }

    flim_info( size_t width, size_t height, bool silent, size_t frame_count, size_t total_ticks, size_t byterate )
        : width_(width), height_(height), silent_(silent),
          frame_count_(frame_count), total_ticks_(total_ticks), byterate_(byterate)
    {
    }

    void serialize( std::vector<uint8_t> &out ) const
    {
        auto o = std::back_inserter(out);
        write2( o, width_ );
        write2( o, height_ );
        write2( o, silent_?1:0 );
        write4( o, frame_count_ );
        write4( o, total_ticks_ );
        write2( o, byterate_ );
    }

    void deserialize( const uint8_t *data, size_t size )
    {
        if (size < 16)
            return;
        const uint8_t *p = data;
        width_ = read2( p );
        height_ = read2( p );
        silent_ = read2( p ) != 0;
        frame_count_ = read4( p );
        total_ticks_ = read4( p );
        byterate_ = read2( p );
    }
};
