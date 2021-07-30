#ifndef IMAGE_INCLUDED__
#define IMAGE_INCLUDED__

#include <cstdint>

#include <vector>
#include <cassert>


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

    image &operator=( const image &o ) = default;

    size_t W() const { return W_; }
    size_t H() const { return H_; }

    const float &at( int x, int y ) const
    {
        assert( x>=0 );
        assert( y>=0 );
        assert( x<W_ );
        assert( y<H_ );
        return image_[x+y*W_];
    }
    float &at( int x, int y )
    {
        assert( x>=0 );
        assert( y>=0 );
        assert( x<W_ );
        assert( y<H_ );
        return image_[x+y*W_];
    }

    enum dithering
    {
        floyd_steinberg = 0,
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
void quantize( image &dest, const image &source, const image &previous, float stability );

bool read_image( image &result, const char *file );
void write_image( const char *file, const image &img );

#include <string>

void watermark( image &img, const std::string &s );

void copy( image &destination, const image &source, bool black_bars=true );


//  #### This has nothing to do here
void delete_files_of_pattern( const std::string &pattern );

#endif
