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
    // image( const image &o ) = default;

    // image()
    // {
    //     for (int y=0;y!=H;y++)
    //         for (int x=0;x!=W;x++)
    //             image_[x][y] = 0;
    // }

    size_t W() const { return W_; }
    size_t H() const { return H_; }

    const float &at( int x, int y ) const
    {
        assert( x<W_ );
        assert( y<H_ );
        return image_[x+y*W_];
    }
    float &at( int x, int y )
    {
        assert( x<W_ );
        assert( y<H_ );
        return image_[x+y*W_];
    }

    image &operator=( const image &o ) = default;

#ifdef STAMP
        //  Code to "stamp" every stream by placing a small number (0 to 7) in the top left
    void stamp( int stamp )
    {
        static int sStamps[8][8] = 
        {
            {0x00,0x38,0x44,0x4c,0x54,0x64,0x44,0x38},
            {0x00,0x10,0x30,0x50,0x10,0x10,0x10,0x38},
            {0x00,0x38,0x44,0x04,0x38,0x40,0x40,0x7c},
            {0x00,0x38,0x44,0x04,0x18,0x04,0x44,0x38},
            {0x00,0x08,0x18,0x28,0x48,0x7c,0x08,0x08},
            {0x00,0x7c,0x40,0x40,0x78,0x04,0x44,0x38},
            {0x00,0x38,0x44,0x40,0x78,0x44,0x44,0x38},
            {0x00,0x7c,0x04,0x04,0x08,0x10,0x10,0x10}
        };

        assert( stamp>=0 && stamp<8 );

        for (int y=0;y!=8;y++)
            for (int x=0;x!=8;x++)
            image_[x+8][y+24] = (sStamps[stamp][y] & (1<<(7-x))) ? 1 : 0;
    }
#endif
};

void fill( image &img, float value = 0.5 );
image round_corners( const image& img );
image filter( const image &from, const char *filters );
void quantize( image &dest, const image &source, const image &previous, float stability );

bool read_image( image &result, const char *file );
void write_image( const char *file, image &img );



#endif
