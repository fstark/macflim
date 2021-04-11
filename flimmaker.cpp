/**
 * The flimmaker tool takes a set of pgm images and generate a flim file suitable for playback by MacFlim on a vintage Mac
 */

#include <stdlib.h>
#include <memory.h>
#include <math.h>
#include <assert.h>
#include <limits>
#include <iostream>
#include <stdio.h>
#include <vector>
#include <array>
#include <memory>
#define noLZG
#ifdef LZG
#include "lzg.h"
#endif

//  True if the global '-g' option was set
bool debug = false;

//  If defined, we add a "stamp" to each stream, to know where it is coming from
#define noSTAMP

#ifdef STAMP
static int sStream = 0;
#endif

//  Almost completely tested implementation of packbits
//  Compresses 'length' bytes from 'buffer' into 'out', and return the compressed size
int packbits( u_int8_t *out, const u_int8_t *buffer, int length )
{
    const u_int8_t *orig = out;
    const u_int8_t *end = buffer+length;

    while (buffer<end)
    {
        //  We look for the next pair of identical characters
        const u_int8_t *next_pair = buffer;
        for (next_pair = buffer;next_pair<end-1;next_pair++)
            if (next_pair[0]==next_pair[1])
                break;

        //  If we didn't find a pair up to the last two chars, we skip to the end
        if (next_pair==end-1)
            next_pair = end;

        //  All character until next_pair don't repeat
        if (next_pair!=buffer)
        {
                //  We have to write len litterals
            u_int32_t len = next_pair-buffer;
            while (len)
            {
                    //  We can write at most 128 literals in one go
                u_int8_t sub_length = len>128?128:len;
                len -= sub_length;
                *out++ = sub_length-1;
                while (sub_length--)
                    *out++ = *buffer++;
            }
        }

        assert( buffer==next_pair );

        //  Now, we are at the start of the next run, or at the end of the stream
        if (buffer==end)
            break;

        assert( buffer<end-1 ); //  As we have a run, we have at least two chars

        u_int8_t c = *buffer;

        //  Find the len of the run
        int len = 0;
        while (*buffer==c)
        {
            len++;
            buffer++;
            if (len==128)
                break;
            if (buffer==end)
                break;
        }

        *out++ = -len+1;
        *out++ = c;

        //  We don't care about the fact that the run may continue, it will be handled by the next loop iteration
    }

    return out-orig;
}

void pack_test()
{
    u_int8_t in0[] =
    {
        0xAA, 0xAA, 0xAA, 0x80, 0x00, 0x2A, 0xAA, 0xAA, 0xAA,
        0xAA, 0x80, 0x00, 0x2A, 0x22, 0xAA, 0xAA, 0xAA, 0xAA,
        0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA
    };
    u_int8_t out0[] =
    {
        0xFE, 0xAA, 0x02, 0x80, 0x00, 0x2A, 0xFD, 0xAA, 0x03,
        0x80, 0x00, 0x2A, 0x22, 0xF7, 0xAA
    };

    u_int8_t buffer[1024];
    int len;
    
    len = packbits( buffer, in0, sizeof(in0) );
    assert( len==sizeof(out0) );
    assert( memcmp( buffer, out0, len )==0 );

    u_int8_t in1[] = {};
    u_int8_t out1[] = {};

    len = packbits( buffer, in1, sizeof(in1) );
    assert( len==sizeof(out1) );
    assert( memcmp( buffer, out1, len )==0 );

    u_int8_t in2[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    };
    u_int8_t out2[] = { 0x81, 0x00, 0xF1, 0x00 };

    len = packbits( buffer, in2, sizeof(in2) );
    assert( len==sizeof(out2) );
    assert( memcmp( buffer, out2, len )==0 );

    u_int8_t in3[] = {
        0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 
        0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 
        0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 
        0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 
        0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 
        0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 
        0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 
        0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 
        0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 
        0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 
        0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 
        0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 
        0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 
        0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 
        0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 
        0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 
        0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 
        0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 
    };

    u_int8_t out3[] = {
        0x7f,
        0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 
        0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 
        0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 
        0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 
        0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 
        0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 
        0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 
        0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 
        0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 
        0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 
        0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 
        0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 
        0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 
        0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 
        0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 
        0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 
        0x0f,
        0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 
        0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 
    };

    len = packbits( buffer, in3, sizeof(in3) );
    assert( len==sizeof(out3) );
    assert( memcmp( buffer, out3, len )==0 );
}



/*
Another packing idea for run-length encoding at bit level. Would be goot for long black or white sequences, but would be awful on 50% grays

0=>7 bits of data
10=> up to 32+7 black
11=> up to 32+7 white

Take next 7 bits
If all equal => encode equal
Else encode specific
*/

/*  Another packing idea, as most of the data is zero anyway
    unsure about the values at the limit...
*/
int packzeroes( u_int8_t *out, const u_int8_t * const buffer, int length )
{
    const u_int8_t *orig = out;
    const u_int8_t *start = buffer;
    const u_int8_t *end = start+length;

    while (start<end)
    {
        //  We look for the next zero
        const u_int8_t *next_zero;
        for (next_zero = start;next_zero<end;next_zero++)
        {
            if (!*next_zero)
                break;
        }

        //  All character until next_zero don't repeat
        if (next_zero!=start)
        {
                //  We have to write len litterals
            u_int32_t len = next_zero-start;
            while (len)
            {
                    //  We can write at most 127 literals in one go
                u_int8_t sub_length = len>127?127:len;
                len -= sub_length;
                *out++ = sub_length;
                while (sub_length--)
                    *out++ = *start++;
            }
        }

        assert( next_zero==start );

        //  Now, we are at the start of the next run of zeroes, or at the end of the stream
        if (start==end)
            break;

        //  Find the len of the run
        int len = 0;
        while (!*start)
        {
            len++;
            start++;
            if (len==128)
                break;
            if (start==end)
                break;
        }
        *out++ = -len;

        //  We don't care about the fact that the run may continue, it will be handled by the next loop iteration
    }

    *out++ = 0;

    // for (int i=0;i!=std::min(32768,length);i++)
    //     printf( "-> %02x ", buffer[i] );
    // printf( "\n" );

    // for (int i=0;i!=std::min(32768L,out-orig);i++)
    //     printf( "<- %02x ", orig[i] );
    // printf( "\n" );

    return out-orig;
}

/*
    Assembly unpack & xor:

*/

//  Screen size constants. MacFlim is supposed to be from a universe where the only framebuffer larger than 512x243 was the Lisa2 / Macintosh XL one.
const int WIDTH = 512;
const int HEIGHT = 342;
const int FB_SIZE = ((WIDTH*HEIGHT)/8);   //  aka 21888

//  This is an image, represented as a bunch of floating point values (0==black and 1==white)
//  Sometime, a pixel can be <0 or >1, when error propagates during dithering
template <int W, int H>
class image
{
    float image_[W][H];
public:
    // image() {}
    // image( const image &o ) = default;

    image &operator=( const image &o ) = default;

    const float *operator[]( int index ) const { return image_[index]; }
    float *operator[]( int index ) { return image_[index]; }
    void *data() const { return (void *)image_; }
    static const size_t encoded_size = (W*H)/8;

#ifdef STAMP
        //  Code to "stamp" every stream by placing a small number in the top left
        //  This helps identify what stream is currently playing 
        //  Disable for release
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

        for (int y=0;y!=8;y++)
            for (int x=0;x!=8;x++)
            image_[x+8][y+24] = (sStamps[stamp][y] & (1<<(7-x))) ? 1 : 0;
    }
#endif

};

//  The two "standard" formats of images
using mac_image = image<WIDTH,HEIGHT>;
using mac_image_small = image<WIDTH/2,HEIGHT/2>;

//  Image copy
template <int W, int H>
void copy_image( image<W,H> &dest, const image<W,H> &source )
{
    memcpy( dest.data(), source.data(), W*H*sizeof(source[0][0]) );
}

//  Fills image with constant color, 50% gray by default
template <int W, int H>
void fill( image<W,H> &img, float value = 0.5 )
{
    for (int y=0;y!=H;y++)
        for (int x=0;x!=W;x++)
            img[x][y] = value;
}

static float g_stability = 0.30;
static float g_max_stability = 1.00;

//  Basic "standard" floyd-steinberg, suitable for static images
//  Dest will only contain 0 or 1, corresponding to the dithering of the source
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


//  Motion floyd-steinberg
//  This will create a black/white 'dest' image from a grayscale 'source'
//  while trying to respect the placement of pixels
//  from the black/white 'previous' image
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
            //  Starting with the current color, including error propageated form previous pixels,
            //  we decide that:
            //  If previous frame pixel was black, we stay back if color<0.5+stability/2
            //  If previous frame pixel was white, we stay white if color>0.5-stability/2
            float color = source_color<=0.5-(previous[x][y]-0.5)*stability2?0:1;
            dest[x][y] = color;

            //  By doing this, we made an error (too much white or too much black)
            //  that we need to keep track of
            float error = source_color - color;

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

template <int W, int H>
image<W,H> sharpen( const image<W,H> &src )
{
    float kernel[3][3] = {
        {  0.0, -1.0,  0.0 },
        { -1.0,  5.0, -1.0 },
        {  0.0, -1.0,  0.0 },
    };

    auto res = src;

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

template <int W, int H>
image<W,H> blur3( const image<W,H> &src )
{
    float kernel[3][3] = {
        { 1.0/9, 1.0/9, 1.0/9 },
        { 1.0/9, 1.0/9, 1.0/9 },
        { 1.0/9, 1.0/9, 1.0/9 },
    };

    auto res = src;

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














int correct( int v )
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

//  Reads an image from a grayscale PGM file of the right size
//  No tests are done, no error are managed, which is shamefull
template <int W, int H>
bool read_image( image<W,H> &result, const char *file )
{
    // fprintf( stderr, "Reading [%s]\n", file );

    image<W,H> image;

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

    //  result = sharpen( image );
    //  result = blur5(blur5(blur5(blur5( image ))));
    // result = sharpen(sharpen(blur5( image )));

    // result = blur5( image );

    result = gamma( image, 1.6 );
    result = sharpen( result );

    return true;
}

//  Generates a PGM image
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

//  Write a bunch of bytes in a file
void write_data( const char *file, u_int8_t *data, size_t len )
{
    // fprintf( stderr, "Writing [%s]\n", file );

    FILE *f = fopen( file, "wb" );

    while (len--)
        fputc( *data++, f );

    fclose( f );
}

//template <int W, int H>
//void diff_image( image<W,H> &diff, image<W,H> &img1, image<W,H> &img2 )
//{
//    for (int y=0;y!=H;y++)
//        for (int x=0;x!=W;x++)
//            diff[x][y] = fabs( img1[x][y] - img2[x][y] );
//}

//  Takes an image, compose of only black and white
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
    static const size_t size = W*H/8;

    typedef enum
    {
        kPackBits = 0,
        kPackZeroes = 1
    } ePack;

private:
    u_int8_t data_[size];

    u_int8_t *packed_data_ = nullptr;
    size_t packed_size_;

    void pack( ePack method )
    {
#ifndef LZG
        u_int8_t buffer[W*H+1];   //  Waaaay too much

        switch (method)
        {
            case kPackBits:
                packed_size_ = ::packbits( buffer, data_, size );
                break;
            case kPackZeroes:
                packed_size_ = ::packzeroes( buffer, data_, size );
                break;
        }

        packed_data_ = new u_int8_t[packed_size_];
        memcpy( packed_data_, buffer, packed_size_ );
#else
        auto maxEncSize = LZG_MaxEncodedSize( size );
        auto buffer = new unsigned char[maxEncSize];
        packed_size_ = LZG_Encode( data_, size, buffer, maxEncSize, NULL );
        packed_data_ = new u_int8_t[packed_size_];
        memcpy( packed_data_, buffer, packed_size_ );
        delete[] buffer;
#endif
    }

    void check_pack( ePack method )
    {
        if (!packed_data_)
            pack( method );
    }
    framebuffer() {}

public:
    framebuffer( const image<W,H> &img )
    {
        auto *p = data_;
        for (int y=0;y!=H;y++)
            for (int x=0;x!=W;x+=8)
                *p++ = (int)(img[x  ][y]*128+img[x+1][y]*64+img[x+2][y]*32+img[x+3][y]*16+
                       img[x+4][y]*  8+img[x+5][y]* 4+img[x+6][y]* 2+img[x+7][y]     ) ^ 0xff;
    }

    framebuffer( const framebuffer &other ) : packed_data_( nullptr )
    {
        memcpy( data_, other.data_, size );
    }

    framebuffer& operator=(const framebuffer& other)
    {
        if (this == &other)
            return *this;
    
        delete[] packed_data_;
        packed_data_ = nullptr;

        return *this;
    }

    u_int8_t *packed_data( ePack method ) { check_pack( method ); return packed_data_; }
    size_t packed_size( ePack method ) { check_pack( method ); return packed_size_; }

    ~framebuffer()
    {
        if (packed_data_)
        {
            // std::cout << (void*)packed_data_ << "\n";
            delete[] packed_data_;
            packed_data_ = nullptr;
        }
    }

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

    std::vector<u_int8_t> packed_data( ePack method ) const
    {
        std::vector<u_int8_t> result;
        auto p = packed_data( method );
        for (size_t i=0;i!=packed_size_;i++)
            result.push_back( *p++ );
        return result;
    }
};

//  Reduce an image to half the size
template <int W, int H>
void reduce_image( image<W,H> &dest, const image<2*W,2*H> &source )
{
    fill( dest, 0 );
    for (int y=0;y!=H;y++)
        for (int x=0;x!=W;x++)
            dest[x][y] = (source[2*x][2*y]+source[2*x+1][2*y]+source[2*x][2*y+1]+source[2*x+1][2*y+1])/4;
}

//  Reduce an image to the same size (ie: copies)
template <int W, int H>
void reduce_image( image<W,H> &dest, const image<W,H> &source )
{
    dest = source;
}

//  Adds a pixel in the 4 corners of the image
template <int W, int H>
void plot4( image<W,H> &img, int x, int y, int c )
{
    img[x][y] = c;
    img[W-1-x][y] = c;
    img[x][H-1-y] = c;
    img[W-1-x][H-1-y] = c;
}

//  Draw a horizonal line in the 4 corners of the image
template <int W, int H>
void hlin4( image<W,H> &img, int x0, int x1, int y, int c )
{
    while (x0<x1)
        plot4( img, x0++, y, c );
}

//  As Steve Jobs asked for Mac desktops to have round corners, forces rounded corners on generated flims.
template <int W, int H>
void round_corners( image<W,H> &img )
{
    hlin4( img, 0, 5, 0, 0 );
    hlin4( img, 0, 3, 1, 0 );
    hlin4( img, 0, 2, 2, 0 );
    hlin4( img, 0, 1, 3, 0 );
    hlin4( img, 0, 1, 4, 0 );
}

//  Encodes a set of N images (N=20)
template <int W, int H, int N>
std::array<std::vector<u_int8_t>,N> compress(
    const image<W,H> &current_image,
    image<W,H> &last,
    const std::array<image<W,H>,N> &source,
    float stability,
    size_t &total,
    size_t &theorical_total,
    std::array<image<W,H>,N> &imgs,
    size_t skip = 0
    )
{
    std::array<std::vector<u_int8_t>,N> result;

    //  We dither the images, using previous as the base
    auto img = current_image;
    float partial = 0;
    for (int i=0;i!=N;i++)
    {
        partial += skip*1.0/N;
        if (partial>1)
        {
            imgs[i] = img;      //  Skip image
            partial -= 1;
        }
        else
            quantize( (imgs)[i], source[i], img, stability );
        img = (imgs)[i];
    }
    //  Return last image so it can be used for encoding of first image of next block
    last = img;

    //  We apply round-corners
    for (auto &img:(imgs))
        round_corners( img );

    //  We convert to framebuffers
    std::vector<framebuffer<W,H>> fbs;
    for (auto &img:(imgs))
        fbs.push_back( framebuffer<W,H>( img ) );

    total = 0;
    theorical_total = 0;

    // framesizes[0] = framebuffer<W,H>::size;
    // auto data = fbs[0].raw_data();
    // result.insert(std::end(result), std::begin(data), std::end(data));

    //  We xor the framebuffers
    for (int i=0;i!=N;i++)
    {
        framebuffer<W,H> a =  i==0?current_image:fbs[i-1];
        framebuffer<W,H> &b = fbs[i];

        auto c = a^b;
//        std::clog << "PACKED == " << c.packed_size() << c::size << "\n";

        auto framesize = c.packed_size( framebuffer<W,H>::kPackZeroes );
        auto data = c.packed_data( framebuffer<W,H>::kPackZeroes );

// std::clog << i << " [" << (int)data[0] << " " << (int)data[1] << "]\n";

        total += 2 + framesize;
        theorical_total += framebuffer<W,H>::size;

        fbs[i] = c;
        result[i] = std::vector<u_int8_t>();
        result[i].push_back( 0 );
        result[i].push_back( 2 );   //  PackByte Zero
        result[i].insert( std::end( result[i] ), data, data+framesize );
    }

    return result;
}

static const int BATCH_SIZE=20;

float compression_target = 1;

template <typename T>
void write1( T &out, int v )
{
    *out++ = v;
}

template <typename T>
void write2( T &out, int v )
{
    write1( out, v/256 );
    write1( out, v%256 );
}

template <typename T>
void write4( T &out, int v )
{
    write2( out, v/65536 );
    write2( out, v%65536 );
}

int8_t *g_soundptr;

template <int W, int H, typename T>
int encode(
    T out,                          //  We we finally append our data
    const std::string &in_arg,      //  File input pattern
    int from_index,                 //  Index to start
    int to_index,                   //  Index to end
    bool dump=true,                 //  Writes resulting images
    int cover_index=-1 )
{
    bool first = true;
    bool failed = false;
    image<W,H> previous_smooth;
    image<W,H> last_smooth;
    mac_image original;             //  512x342, float, 0 and 1 only
    image<W,H> original_scaled;     //  May be smaller
    size_t frame_count = 0;

    std::unique_ptr<std::array<image<W,H>,BATCH_SIZE>> source;
    source = std::make_unique<std::array<image<W,H>,BATCH_SIZE>>();
    std::unique_ptr<std::array<image<W,H>,BATCH_SIZE>> result;
    result = std::make_unique<std::array<image<W,H>,BATCH_SIZE>>();

    for (int blk=from_index;blk<=to_index;blk+=BATCH_SIZE)
    {
            //  Loads all the images into 'source'
        for (int j=0;j!=BATCH_SIZE;j++)
        {
            int i = blk + j;
            char buffer[1024];
            sprintf( buffer, in_arg.c_str(), i );

            if (!read_image( original, buffer ))
            {
                // We ignore, and keep the previous original
                // std::clog << "Adding frame\n";
                failed = true;
            }

            if (first)
            {
                fill( previous_smooth, 0 );
                first = false;
            }

                //  Reduce imag if needed
            reduce_image( (*source)[j], original );
        }

            //  Source contains the images

            //  We compress until we are happy with the result
        size_t target = BATCH_SIZE*W*H/8*compression_target;   //  compression target
        size_t total;
        float stability;
        size_t theorical_total;
        std::array<std::vector<u_int8_t>,BATCH_SIZE> packed_frames;

        int skip;

        for (skip=0;skip!=BATCH_SIZE;skip++)
        {
            float first_stability = g_stability;
            float last_stability = g_max_stability;

            //  We test at max to know if it is even worth looping
            stability = g_max_stability;
            packed_frames = compress<W,H,BATCH_SIZE>( previous_smooth, last_smooth, (*source), stability, total, theorical_total, *result, skip );
            if (total>target)
                continue;

            while (first_stability+.05<=last_stability)
            {
                stability = (first_stability+last_stability)/2;
                packed_frames = compress<W,H,BATCH_SIZE>( previous_smooth, last_smooth, (*source), stability, total, theorical_total, *result, skip );
                std::clog << blk << ": " << skip << " " << (float)total/theorical_total*100.0 << "\r";
                if (total<target)
                    last_stability = stability;
                else
                    first_stability = stability;
            }

            if (total<=target)
                break;

            // do
            // {
            //     packed_frames = compress<W,H,BATCH_SIZE>( previous_smooth, last_smooth, (*source), stability, total, theorical_total, *result, skip );
            //     std::clog << blk << ": " << skip << " " << (float)total/theorical_total*100.0 << "\r";
            //     if (total<=target)
            //         goto done;
            //     stability += 0.05;
            // }   while (stability<=g_max_stability+0.05);
        }

done:
        // stability -= 0.05;

        fprintf( stderr, "%4d => %1.02f/%02d %6.02f%% %6ld ", blk, stability, skip, (float)total/theorical_total*100.0, total );

            //  We are happy, packed_frames contains the packed data for the frames

        // std::clog << blk << "=> " << ":" << stability << "(" << (float)total/theorical_total*100.0 << ") " << total << " ";

        copy_image( previous_smooth, last_smooth );

        //  Output sequence
        for (int j=0;j!=BATCH_SIZE;j++)
        {
            int i = blk + j;

            //  Dump i
            if (dump)
            {
                char buffer[1024];
                sprintf( buffer, "out-%06d.pgm", i );
                write_image( buffer, (*result)[j] );
            }

            if (cover_index!=-1 && i>=cover_index && i<cover_index+24)
            {
                if (debug)
                    std::clog << "COVER AT " << i << "\n";
                char buffer[1024];

                sprintf( buffer, "cover-%06d.pgm", i-cover_index );
                write_image( buffer, (*result)[j] );
            }

            frame_count++;
        }

        // encode( out, (*result)[j] );
        size_t SOUND_LEN = 370*BATCH_SIZE*2;

        write4( out, 4+4+4+6+SOUND_LEN +2+2*BATCH_SIZE+total );
        write4( out, 0x41424344 );
        fprintf( stderr, " BLOCK_SIZE = %7ld, ", 4+4+ 6+SOUND_LEN +2+2*BATCH_SIZE+total );
        write4( out, 6+SOUND_LEN );
        write2( out, 0 );       //  ffMode
        write4( out, 65536 );   //  Fixed(1,1)
        for (int i=0;i!=SOUND_LEN;i++)
//            write1( out, (i%74)<37?0:127 );
            write1( out, *g_soundptr++ );
        write2( out, BATCH_SIZE );
        fprintf( stderr, "  FRAMES : " );
        for (auto v:packed_frames)
        {
            write2( out, v.size() );
            fprintf( stderr, "%5ld ", v.size() );
            for (auto b:v)
            {
                write1( out, b );
                // printf( " %02x ", b );
            }
            // printf( "\n" );
        }
        fprintf( stderr, "\n" );

        if (failed)
            break;
    }

    write4( out, 0 );

    return frame_count;
}

//  The main function, does all the work
//  flimmaker [-g] --in <%d.pgm> --from <index> --to <index> --cover <index> --audio <audio.waw> --out <file>
int main( int argc, char **argv )
{
    std::string in_arg = "movie-%06d.pgm";
    std::string out_arg = "out.flim";
    std::string audio_arg = "audio.wav";
    int from_index = 1;
    int to_index = std::numeric_limits<int>::max();
    int cover_index = -1;

    argc--;
    argv++;

    while (argc)
    {
        if (!strcmp(*argv,"--compression-target"))
        {
            argc--;
            argv++;
            compression_target = atof(*argv)/100.0;
            argc--;
            argv++;
        }
        else if (!strcmp(*argv,"-g"))
        {
            debug = true;
            argc--;
            argv++;
        }
        else if (!strcmp(*argv,"--in"))
        {
            argc--;
            argv++;
            in_arg = *argv;
            argc--;
            argv++;
        }
        else if (!strcmp(*argv,"--from"))
        {
            argc--;
            argv++;
            from_index = atoi(*argv);
            argc--;
            argv++;
        }
        else if (!strcmp(*argv,"--to"))
        {
            argc--;
            argv++;
            to_index = atoi(*argv);
            argc--;
            argv++;
        }
        else if (!strcmp(*argv,"--cover"))
        {
            argc--;
            argv++;
            cover_index = atoi(*argv);
            argc--;
            argv++;
        }
        else if (!strcmp(*argv,"--audio"))
        {
            argc--;
            argv++;
            audio_arg = *argv;
            argc--;
            argv++;
        }
        else if (!strcmp(*argv,"--stability"))
        {
            argc--;
            argv++;
            g_stability = atof( *argv );
            argc--;
            argv++;
        }
        else if (!strcmp(*argv,"--max-stability"))
        {
            argc--;
            argv++;
            g_max_stability = atof( *argv );
            argc--;
            argv++;
        }
        else if (!strcmp(*argv,"--out"))
        {
            argc--;
            argv++;
            out_arg = *argv;
            argc--;
            argv++;
        }
        else
        {
            std::cerr << "Unknown argument " << *argv << "\n";
            return EXIT_FAILURE;
        }
    }

    //  Load audio
    int8_t *audio_buffer;
    long audio_size;
    FILE *f = fopen( audio_arg.c_str(), "rb" );
    if (f)
    {
        fseek( f, 0L, SEEK_END );
        audio_size = ftell( f );
        rewind( f );
        audio_buffer = (int8_t *)malloc( audio_size ) ;
        if (fread( audio_buffer, audio_size, 1, f )!=1)
            std::cerr << "**** WARNING: CANNOT READ AUDIO FILE [" << audio_arg << "]\n";
        fclose( f );
    }
    else
        std::cerr << "**** ERROR: CANNOT OPEN AUDIO FILE [" << audio_arg << "]\n";

    g_soundptr = audio_buffer;

    if (cover_index==-1)
        cover_index = from_index+(to_index-from_index)/3;

//    pack_test();


    std::vector<u_int8_t> data;
    int count;
    count = encode<512,342>( std::back_inserter(data), in_arg, from_index, to_index, true, cover_index );

    if (g_soundptr!=audio_buffer+audio_size)
        fprintf( stderr, "???? SOUND ENCODED %ld samples instead of %ld\n", g_soundptr-audio_buffer, audio_size );

    FILE *movie_file = fopen( out_arg.c_str(), "wb" );
    // fwrite( res.data(), res.size(), 1, movie_file );
    fwrite( data.data(), data.size(), 1, movie_file );
    fclose( movie_file );

    return EXIT_SUCCESS;
}
