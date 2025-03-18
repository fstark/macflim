#pragma once

#include <cstdint>

#include <vector>
#include <string>
#include <cassert>

#include <functional>

#include <algorithm>
#include "common.hpp"

//  ------------------------------------------------------------------
//  An image class and various associated utilities
//  ------------------------------------------------------------------

//  This is an image, represented as a bunch of floating point values (0==black and 1==white)
//  Sometime, a pixel can be <0 or >1, when error propagates during dithering
class image
{
public:
    std::vector<float> image_;
    size_t W_;
    size_t H_;

public:
    image( size_t W, size_t H ) : image_( W*H ), W_{W}, H_{H}
    {
    }

    size_t W() const { return W_; }
    size_t H() const { return H_; }

    const float &at( size_t x, size_t y ) const
    {
        assert( x < W_ );
        assert( y < H_ );
        assert( x<W_ );
        assert( y<H_ );
        return image_[x+y*W_];
    }
    float &at( size_t x, size_t y )
    {
        assert( x < W_ );
        assert( y < H_ );
        assert( x<W_ );
        assert( y<H_ );
        return image_[x+y*W_];
    }

    enum dithering
    {
        error_diffusion = 0,
        ordered = 1
    };

    //  Dealing with ffmpeg data
    
    void set_luma( const uint8_t *y )  //  Sets the (monochrome) content from a Y luma buffer of W*H bytes
    {
        std::transform( y, y+W_*H_, std::begin(image_), []( auto v ) { return v/255.0; } );
    }
};

void fill( image &img, float value = 0.5 );
image round_corners( const image& img );
image filter( const image &from, const char *filters );
void ordered_dither( image &dest, const image &source, const image &previous );

struct dither_algorithm;

const dither_algorithm *get_error_diffusion_by_name( const std::string &name );
void error_diffusion_algorithms( std::function<void(const std::string name, const std::string desciption)> f );
void error_diffusion( image &dest, const image &source, const image &previous, float stability, const dither_algorithm &algo, float bleed=1, bool two_ways=false );

bool read_image( image &result, const char *file );
void write_image( const char *file, const image &img );

void copy( image &destination, const image &source, bool black_bars=true );

