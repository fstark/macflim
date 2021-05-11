#ifndef IMAGE_INCLUDED__
#define IMAGE_INCLUDED__

#include <cstdint>
#include "imgcompress.hpp"

/** Image manipulation classes */

//  ------------------------------------------------------------------
//  Screen size constants. MacFlim is supposed to be from a universe where the only framebuffer larger than 512x342 was the Lisa2 / Macintosh XL one.
//  ------------------------------------------------------------------

const int WIDTH = 512;
const int HEIGHT = 342;
const int FB_SIZE = ((WIDTH*HEIGHT)/8);   //  aka 21888

//  ------------------------------------------------------------------
//  An image class and various associated utilities
//  ------------------------------------------------------------------

//  This is an image, represented as a bunch of floating point values (0==black and 1==white)
//  Sometime, a pixel can be <0 or >1, when error propagates during dithering
template <int W, int H>
class image
{
    float image_[W][H];
public:
    // image() {}
    // image( const image &o ) = default;

    // image()
    // {
    //     for (int y=0;y!=H;y++)
    //         for (int x=0;x!=W;x++)
    //             image_[x][y] = 0;
    // }

    image &operator=( const image &o ) = default;

    const float *operator[]( int index ) const { return image_[index]; }
    float *operator[]( int index ) { return image_[index]; }
    void *data() const { return (void *)image_; }
    static const size_t encoded_size = (W*H)/8;

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

//  ------------------------------------------------------------------
//  The two "standard" formats of images
//  ------------------------------------------------------------------
using mac_image = image<WIDTH,HEIGHT>;
using mac_image_small = image<WIDTH/2,HEIGHT/2>;

//  ------------------------------------------------------------------
//  Copy image (#### : is operator=?)
//  ------------------------------------------------------------------
template <int W, int H>
void copy_image( image<W,H> &dest, const image<W,H> &source )
{
    memcpy( dest.data(), source.data(), W*H*sizeof(source[0][0]) );
}

//  ------------------------------------------------------------------
//  Fills image with constant color, 50% gray by default
//  ------------------------------------------------------------------
template <int W, int H>
void fill( image<W,H> &img, float value = 0.5 )
{
    for (int y=0;y!=H;y++)
        for (int x=0;x!=W;x++)
            img[x][y] = value;
}

//  ------------------------------------------------------------------
//  Sharpens the image
//  ------------------------------------------------------------------
template <int W, int H>
image<W,H> sharpen( const image<W,H> &src )
{
    float kernel[3][3] = {
        {  0.0, -1.0,  0.0 },
        { -1.0,  5.0, -1.0 },
        {  0.0, -1.0,  0.0 },
    };

    auto res = src;

    for (int x=0;x!=W;x++)
        res[x][0] = res[x][H-1] = 0;
    for (int y=0;y!=H;y++)
        res[0][y] = res[W-1][y] = 0;

    fill( res, 0 );

    for (int x=1;x!=W-1;x++)
        for (int y=1;y!=H-1;y++)
        {
            float v = 0;
            for (int x0=0;x0!=3;x0++)
                for (int y0=0;y0!=3;y0++)
                {
                    v += src[x+x0-1][y+y0-1]*kernel[x0][y0];
                }
            res[x][y] = v;
        }

    return res;
}

//  ------------------------------------------------------------------
//  Blurs the image with a 3x3 kernel
//  ------------------------------------------------------------------
template <int W, int H>
image<W,H> blur3( const image<W,H> &src )
{
    static float kernel[3][3] = {
        { 1.0/9, 1.0/9, 1.0/9 },
        { 1.0/9, 1.0/9, 1.0/9 },
        { 1.0/9, 1.0/9, 1.0/9 },
    };

    image<W,H> res;

    // for (int x=0;x!=W;x++)
    //     res[x][0] = res[x][H-1] = 0;
    // for (int y=0;y!=H;y++)
    //     res[0][y] = res[W-1][y] = 0;

    fill( res, 0 );

    for (int x=1;x!=W-1;x++)
        for (int y=1;y!=H-1;y++)
        {
            float v = 0;
            for (int x0=0;x0!=3;x0++)
                for (int y0=0;y0!=3;y0++)
                {
                    v += src[x+x0-1][y+y0-1]*kernel[x0][y0];
                }
            if (v<0) v = 0;
            if (v>1) v = 1;
            res[x][y] = v;
        }

    return res;
}

//  ------------------------------------------------------------------
//  Blurs the image more with a 5x5 kernel
//  ------------------------------------------------------------------
template <int W, int H>
image<W,H> blur5( const image<W,H> &src )
{
    float kernel[5][5] = {
        { 1.0/256, 4.0/256, 6.0/256, 4.0/256, 1.0/256},
        { 4.0/256,16.0/256,24.0/256,16.0/256, 4.0/256},
        { 6.0/256,24.0/256,36.0/256,24.0/256, 6.0/256},
        { 4.0/256,16.0/256,24.0/256,16.0/256, 4.0/256},
        { 1.0/256, 4.0/256, 6.0/256, 4.0/256, 1.0/256},
    };

    auto res = src;

    for (int x=0;x!=W;x++)
        res[x][0] = res[x][H-1] = res[x][1] = res[x][H-2] = 0;
    for (int y=0;y!=H;y++)
        res[0][y] = res[W-1][y] = res[1][y] = res[W-2][y] = 0;

    fill( res, 0 );

    for (int x=2;x!=W-2;x++)
        for (int y=2;y!=H-2;y++)
        {
            float v = 0;
            for (int x0=0;x0!=5;x0++)
                for (int y0=0;y0!=5;y0++)
                {
                    v += src[x+x0-2][y+y0-2]*kernel[x0][y0];
                }
            res[x][y] = v;
        }

    return res;
}

//  ------------------------------------------------------------------
//  Gamma corrects the image
//  ------------------------------------------------------------------
template <int W, int H>
image<W,H> gamma( const image<W,H> &src, double gamma )
{
    image<W,H> res;

    for (int x=0;x!=W;x++)
        for (int y=0;y!=H;y++)
        {
            res[x][y] = pow( src[x][y], gamma );
        }

    return res;
}

//  ------------------------------------------------------------------
template <int W, int H>
image<W,H> zoom_out( const image<W,H> &src )
{
    const double bx = 32;
    const double a = ((W/2)-bx)/(W/2);
    const double by = H/2-a*(H/2);

    image<W,H> res;
    for (int y=0;y!=H;y++)
        for (int x=0;x!=W;x++)
        {
            int from_x = (x-bx)/a;
            int from_y = (y-by)/a;

            if (from_x>0 && from_x<W && from_y>0 && from_y<H)
                res[x][y] = src[from_x][from_y];
            else
                res[x][y] = 0;
        }
    
    return res;
}

//  ------------------------------------------------------------------
//  Reduce an image to half the size
//  ------------------------------------------------------------------
template <int W, int H>
void reduce_image( image<W,H> &dest, const image<2*W,2*H> &source )
{
    fill( dest, 0 );
    for (int y=0;y!=H;y++)
        for (int x=0;x!=W;x++)
            dest[x][y] = (source[2*x][2*y]+source[2*x+1][2*y]+source[2*x][2*y+1]+source[2*x+1][2*y+1])/4;
}

//  ------------------------------------------------------------------
//  Reduce an image to the same size (ie: copies)
//  ------------------------------------------------------------------
template <int W, int H>
void reduce_image( image<W,H> &dest, const image<W,H> &source )
{
    dest = source;
}

//  ------------------------------------------------------------------
//  Adds a pixel in the 4 corners of the image
//  ------------------------------------------------------------------
template <int W, int H>
void plot4( image<W,H> &img, int x, int y, int c )
{
    img[x][y] = c;
    img[W-1-x][y] = c;
    img[x][H-1-y] = c;
    img[W-1-x][H-1-y] = c;
}

//  ------------------------------------------------------------------
//  Draw a horizonal line in the 4 corners of the image
//  ------------------------------------------------------------------
template <int W, int H>
void hlin4( image<W,H> &img, int x0, int x1, int y, int c )
{
    while (x0<x1)
        plot4( img, x0++, y, c );
}

//  ------------------------------------------------------------------
//  As Steve Jobs asked for Mac desktops to have round corners, forces rounded corners on generated flims.
//  ------------------------------------------------------------------
template <int W, int H>
void round_corners( image<W,H> &img )
{
    hlin4( img, 0, 5, 0, 0 );
    hlin4( img, 0, 3, 1, 0 );
    hlin4( img, 0, 2, 2, 0 );
    hlin4( img, 0, 1, 3, 0 );
    hlin4( img, 0, 1, 4, 0 );
}

template <int W, int H>
image<W,H> round_corners( const image<W,H> &img )
{
    image<W,H> res = img;
    round_corners( res );
    return res;
}

template <int W, int H>
image<W,H> quantize( const image<W,H> &img, int n )
{
    image<W,H> res;

    for (int y=0;y!=H;y++)
        for (int x=0;x!=W;x++)
            res[x][y] = ((int)(img[x][y]*(n-1)+.5))/(double)(n-1);

    return res;
}

//  ------------------------------------------------------------------
//  Apply a filter on an image
//  ------------------------------------------------------------------
typedef enum
{
    kBlur3 = 'b',
    kBlur5 = 'B',
    kSharpen = 's',
    kGamma = 'g',
    kRoundCorners = 'c',
    kZoomOut = 'z',
    kQuantize16 = 'Q'
}   eFilters;

template <int W, int H>
image<W,H> filter( const image<W,H> &from, eFilters filter, double arg=0 )
{
    switch (filter)
    {
        case kBlur3:
            return blur3( from );
        case kBlur5:
            return blur5( from );
        case kSharpen:
            return sharpen( from );
        case kGamma:
            return gamma( from, arg?arg:1.6 );
        case kRoundCorners:
            return round_corners( from );
        case kZoomOut:
            return zoom_out( from );
        case kQuantize16:
            return quantize( from, arg?arg:16 );
    }
    std::cerr << "**** ERROR: filter ['" << (char)filter << "'] (" << (int)filter << ") unknown\n";
    throw "Unknown filter";
}

inline bool extract_filter( const char *&p, char &f, double &arg )
{
    arg = 0;
    if (!*p)
        return 0;

        //  Filter name
    f = *p++;

        //  Filter argument (int)
    while (*p>='0' && *p<='9')
        arg = arg*10+ (*p++)-'0';

        //  Decimal part of filter argument
    if (*p=='.')
    {
        p++;
        double scale = 0.1;
        while (*p>='0' && *p<='9')
        {
            arg += ((*p++)-'0')*scale;
            scale /= 10;
        }
    }

    // printf( "filter==%c arg==%f\n", f, arg );

    return true;
}

//  ------------------------------------------------------------------
//  Apply a sequence of filters
//  ------------------------------------------------------------------

template <int W, int H>
image<W,H> filter( const image<W,H> &from, const char *filters )
{
    image<W,H> res = from;
    char f;
    double arg;

    while (extract_filter( filters, f, arg ))
        res = filter( res, (eFilters)f, arg );

    return res;
}

//  ------------------------------------------------------------------
//  Reads an image from a grayscale PGM file of the right size
//  ------------------------------------------------------------------
inline int correct( int v )
{
    if (v<=0x01)
        v = 0;
    if (v>=0xfc)
        v = 0xff;
    return v;
}

// int correct( int v )
// {
//     v = ((v+8)/16)*16;
//     if (v>=255)
//         v = 255;
//     return v;
// }

template <int W, int H>
bool read_image( image<W,H> &result, const char *file )
{
    // fprintf( stderr, "Reading [%s]\n", file );

    image<W,H> image;
    fill( image, 0 );

    FILE *f = fopen( file, "rb" );

    if (!f)
        return false;

    for (int i=0;i!=15;i++)
        fgetc( f );

    for (int y=0;y!=H;y++)
        for (int x=0;x!=W;x++)
        {    // img[x][y] = ((int)(fgetc(f)/255.0*16))/16.0;
            image[x][y] = correct( fgetc(f) )/255.0;
        }

    fclose( f );

    result = image;

    //  result = sharpen( image );
    //  result = blur5(blur5(blur5(blur5( image ))));
    // result = sharpen(sharpen(blur5( image )));

    // result = blur5( image );

//    result = gamma( image, 1.6 );
//    result = blur5( image );
//    result = sharpen( result );
//    result = sharpen( result );
//    result = zoom_out( result );
    //  result = filter( image, "gs" );

    return true;
}

//  ------------------------------------------------------------------
//  Generates a PGM image
//  ------------------------------------------------------------------
template <int W, int H>
void write_image( const char *file, image<W,H> &img )
{
    // fprintf( stderr, "Writing [%s]\n", file );

    FILE *f = fopen( file, "wb" );

    fprintf( f, "P5\n%d %d\n255\n", W, H );
    for (int y=0;y!=H;y++)
        for (int x=0;x!=W;x++)
            fputc( img[x][y]*255, f );

    fclose( f );
}



//  ------------------------------------------------------------------
//  Basic "standard" floyd-steinberg, suitable for static images
//  Dest will only contain 0 or 1, corresponding to the dithering of the source
//  Here for reference, not used
//  ------------------------------------------------------------------
template <int W, int H>
void quantize( image<W,H> &dest, const image<W,H> &source )
{
    copy_image( dest, source );

    for (int y=0;y!=H;y++)
        for (int x=0;x!=W;x++)
        {
            float source_color = dest[x][y];
            float color;
            float error;
            color = source_color<=0.5?0:1;
            error = source_color - color;
            dest[x][y] = color;
            float e0 = error * 7 / 16;
            float e1 = error * 3 / 16;
            float e2 = error * 5 / 16;
            float e3 = error * 1 / 16;
            if (x<W-1)
                dest[x+1][y] = dest[x+1][y] + e0;
            if (x>0 && y<H-1)
                dest[x-1][y+1] = dest[x-1][y+1] + e1;
            if (y<H-1)
                dest[x][y+1] = dest[x][y+1] + e2;
            if (x<W-1 && y<H-1)
                dest[x+1][y+1] = dest[x+1][y+1] + e3;
        }
}

static int dither[8][8] =
{
    { 0, 32, 8, 40, 2, 34, 10, 42},
    {48, 16, 56, 24, 50, 18, 58, 26},
    {12, 44, 4, 36, 14, 46, 6, 38},
    {60, 28, 52, 20, 62, 30, 54, 22},
    { 3, 35, 11, 43, 1, 33, 9, 41},
    {51, 19, 59, 27, 49, 17, 57, 25},
    {15, 47, 7, 39, 13, 45, 5, 37},
    {63, 31, 55, 23, 61, 29, 53, 21}
};

template <int W, int H>
void quantize2( image<W,H> &dest, const image<W,H> &source, const image<W,H> &previous, float stability )
{
    for (int y=0;y!=H;y++)
        for (int x=0;x!=W;x++)
        {
            //  The color we'd like this pixel to be
            float color = source[x][y];

            //  We look at our position in the dithering matrix
            int xd = x%8;
            int yd = y%8;

            //  We look if we are in the threshold
            bool threshold = dither[xd][yd]<color*64;

            if (threshold)
                dest[x][y] = 1;
            else
                dest[x][y] = 0;
        }
}

//  ------------------------------------------------------------------
//  Motion floyd-steinberg
//  This will create a black/white 'dest' image from a grayscale 'source'
//  while trying to respect the placement of pixels
//  from the black/white 'previous' image
//  ------------------------------------------------------------------
template <int W, int H>
void quantize( image<W,H> &dest, const image<W,H> &source, const image<W,H> &previous, float stability )
{
    copy_image( dest, source );

    for (int y=0;y!=H;y++)
        for (int x=0;x!=W;x++)
        {
            //  The color we'd like this pixel to be
            float source_color = dest[x][y];

            //  Increasing the stability value will makes the image choose previous frame's pixel more often
            //  Images will be "stable", but there will be some "ghosting artifacts"

            double stability2 = stability;

/*
            //  We lookup at the real color we are targetting
            float real_color = source[x][y];

            //  We look at our position in the dithering matrix
            int xd = x%8;
            int yd = y%8;

            //  We look if we are in the threshold
            bool threshold = dither[xd][yd]<real_color*63;

            if (threshold)
                stability2 *= 1.5;
            else
                stability2 /= 1.5;
*/

            //  We chose either back or white for this pixel
            //  Starting with the current color, including error propagated form previous pixels,
            //  we decide that:
            //  If previous frame pixel was black, we stay back if color<0.5+stability/2
            //  If previous frame pixel was white, we stay white if color>0.5-stability/2
            float color = source_color<=0.5-(previous[x][y]-0.5)*stability2?0:1;
            dest[x][y] = color;

            //  By doing this, we made an error (too much white or too much black)
            //  that we need to keep track of
            float error = source_color - color;

            // if (fabs(error)<0.10)
            //     error = 0;

            //  We now distribute the error between the 4 next values
            //  (if they exist). The values may over or underflow
            //  but it is fine as pixels can be <0 or >1
            float e0 = error * 7 / 16;
            float e1 = error * 3 / 16;
            float e2 = error * 5 / 16;
            float e3 = error * 1 / 16;

            if (x<W-1)
                dest[x+1][y] = dest[x+1][y] + e0;
            if (x>0 && y<H-1)
                dest[x-1][y+1] = dest[x-1][y+1] + e1;
            if (y<H-1)
                dest[x][y+1] = dest[x][y+1] + e2;
            if (x<W-1 && y<H-1)
                dest[x+1][y+1] = dest[x+1][y+1] + e3;
        }
}

//  Takes an image, composed of only black and white
//  and generates W*H/8 bytes corresponding to a Macintosh framebuffer
template <int W, int H, typename T>
void encode( T out, const image<W,H> &img )
{
    // static char c = 0;
    for (int y=0;y!=H;y++)
        for (int x=0;x!=W;x+=8)
            *out++ = (int)(img[x  ][y]*128+img[x+1][y]*64+img[x+2][y]*32+img[x+3][y]*16+
                      img[x+4][y]*  8+img[x+5][y]* 4+img[x+6][y]* 2+img[x+7][y]     ) ^ 0xff;
}

template <int W,int H>
class framebuffer
{
public:
    static const size_t row_bytes = W/8;
    static const size_t size = H*row_bytes;
    static const size_t lsize = size/sizeof(uint32_t);

private:
    u_int8_t data_[size];

public:

    std::array<uint32_t,lsize> raw32_horizontal() const
    {
        std::array<uint32_t,lsize> res;
        for (int i=0;i!=lsize;i++)
        {
            res[i] = data_[i*4];
            res[i] = (res[i]<<8) + data_[i*4+1];
            res[i] = (res[i]<<8) + data_[i*4+2];
            res[i] = (res[i]<<8) + data_[i*4+3];
        }
        return res;
    }

    std::array<uint32_t,lsize> raw32_vertical() const
    {
        std::array<uint32_t,lsize> res;
        for (int i=0;i!=lsize;i++)
        {
            size_t x = (i/H)*4;
            size_t y = i%H;

            size_t offset = x+y*row_bytes;

assert( x<row_bytes );
assert( y<H );
assert( offset<21888 );

            res[i] = data_[offset];
            res[i] = (res[i]<<8) + data_[offset+1];
            res[i] = (res[i]<<8) + data_[offset+2];
            res[i] = (res[i]<<8) + data_[offset+3];
        }
        return res;
    }

    std::array<uint32_t,lsize> raw32() const
    {
        return raw32_vertical();
    }

    framebuffer( const std::array<uint32_t,lsize> &data )
    {
        for (int i=0;i!=lsize;i++)
        {
            size_t x = (i/H)*4;
            size_t y = i%H;

            size_t offset = x+y*row_bytes;

            data_[offset] = data[i]>>24;
            data_[offset+1] = data[i]>>16;
            data_[offset+2] = data[i]>>8;
            data_[offset+3] = data[i];
        }
    }

    framebuffer()
    {
        memset( data_, 0, size );
    }

     ~framebuffer()
    {
    }

   framebuffer( const image<W,H> &img )
    {
        auto *p = data_;
        for (int y=0;y!=H;y++)
            for (int x=0;x!=W;x+=8)
                *p++ = (int)(img[x  ][y]*128+img[x+1][y]*64+img[x+2][y]*32+img[x+3][y]*16+
                       img[x+4][y]*  8+img[x+5][y]* 4+img[x+6][y]* 2+img[x+7][y]     ) ^ 0xff;
    }

    image<W,H> as_image() const
    {
        image<W,H> res;
        for (int y=0;y!=H;y++)
            for (int x=0;x!=W;x++)
                res[x][y] = !(data_[y*row_bytes+x/8] & (1<<(7-(x%8))));
        return res;
    }

    // framebuffer( const framebuffer &other )
    // {
    //     memcpy( data_, other.data_, size );
    // }

    // framebuffer& operator=(const framebuffer& other)
    // {
    //     if (this == &other)
    //         return *this;
    
    //     return *this;
    // }

    framebuffer operator^(const framebuffer &o)
    {
        framebuffer result;
        for (size_t i=0;i!=size;i++)
            result.data_[i] = data_[i] ^ o.data_[i];
        return result;
    }

    std::vector<u_int8_t> raw_data() const
    {
        std::vector<u_int8_t> result;
        for (size_t i=0;i!=size;i++)
            result.push_back( data_[i] );
        return result;
    }
};


template <int W, int H>
framebuffer<W,H> convert( const image<W,H> &source, const framebuffer<W,H> &previous_fb, float stability )
{
    image<W,H> dest;
    image<W,H> previous = previous_fb.as_image();
    quantize( dest, source, previous, stability );
    return framebuffer<W,H>{ dest };
}

#endif
