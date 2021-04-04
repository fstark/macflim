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

//  True if the global '-g' option was set
bool debug = false;

//  If defined, we add a "stamp" to each stream, to know where it is coming from
#define noSTAMP

#ifdef STAMP
static int sStream = 0;
#endif

#if 0

//  This part contains some compression routines that were used in a previous version.
//  They will be used again in a future version, for sure

//  Almost completely tested implementation of packbits
int pack( u_int8_t *out, const u_int8_t *buffer, int length )
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
    
    len = pack( buffer, in0, sizeof(in0) );
    assert( len==sizeof(out0) );
    assert( memcmp( buffer, out0, len )==0 );

    u_int8_t in1[] = {};
    u_int8_t out1[] = {};

    len = pack( buffer, in1, sizeof(in1) );
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

    len = pack( buffer, in2, sizeof(in2) );
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

    len = pack( buffer, in3, sizeof(in3) );
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


#endif

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

//  Motion floyd-steinberg
//  This will create a black/white 'dest' image from a grayscale 'source'
//  while trying to respect the placement of pixels
//  from the black/white 'previous' image
template <int W, int H>
void quantize( image<W,H> &dest, const image<W,H> &source, const image<W,H> &previous )
{
    copy_image( dest, source );

    for (int y=0;y!=H;y++)
        for (int x=0;x!=W;x++)
        {
            //  The color we'd like this pixel to be
            float source_color = dest[x][y];

            //  Increasing this value will makes the image choose previous frame's pixel more often
            //  Images will be "stable", but there will be some "ghosting artifacts"
            const float stability = 0.3;

            //  We chose either back or white for this pixel
            //  Starting with the current color, including error propageated form previous pixels,
            //  we decide that:
            //  If previous frame pixel was black, we stay back if color<0.65
            //  If previous frame pixel was white, we stay white if color>0.35
            float color = source_color<=0.5-(previous[x][y]-0.5)*stability?0:1;
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

//  Reads an image from a grayscale PGM file of the right size
//  No tests are done, no error are managed, which is shamefull
template <int W, int H>
bool read_image( image<W,H> &img, const char *file )
{
    // fprintf( stderr, "Reading [%s]\n", file );

    FILE *f = fopen( file, "rb" );

    if (!f)
        return false;

    for (int i=0;i!=15;i++)
        fgetc( f );

    for (int y=0;y!=H;y++)
        for (int x=0;x!=W;x++)
            // img[x][y] = ((int)(fgetc(f)/255.0*16))/16.0;
            img[x][y] = fgetc(f)/255.0;

    fclose( f );

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

//  Encodes a set of files at the correct size
//  If skip is 2, only half of the images will be used
//  Encoded result writen into the out iterator
template <int W, int H, typename T>
int encode( T out, const std::string &in_arg, int from_index, int to_index, int skip=1, bool dump=false, int cover_index=-1 )
{
    mac_image original; //  512x342
    image<W,H> source;
    image<W,H> dest_smooth;
    image<W,H> previous_smooth;
    fill( previous_smooth, 0 );

    int frame_count = 0;

    for (int i=from_index;i<=to_index;i+=skip)
    {
        char buffer[1024];
        sprintf( buffer, in_arg.c_str(), i );
        if (!read_image( original, buffer ))
        {
            std::clog << "Stopped read at [" << buffer << "]\n";

            int added = 0;
            while (frame_count%20)
            {
                encode( out, dest_smooth );
                frame_count++;
                added++;
            }
            if (added)
                std::clog << "Added " << added << " frames\n";
            return frame_count;
        }

        reduce_image( source, original );

#ifdef STAMP
        source.stamp( sStream );
#endif

        if (i==from_index)
        {
                //  We quantize the first image alone
            quantize( dest_smooth, source );
        }
        else
        {  
                //  We use the previous image for temporal stability
            quantize( dest_smooth, source, previous_smooth );
        }

        round_corners( dest_smooth );

#ifdef STAMP
        //  Useful for debugging, adds a progress-bar like line of pixels in the images
        //  So one can know where an image should be in the stream

        for (int x=0;x!=i%W;x++)
        {
            dest_smooth[x][49] = 0;
            dest_smooth[x][50] = 1;
            dest_smooth[x][51] = 0;
        }
        for (int x=i%W;x!=W;x++)
        {
            dest_smooth[x][49] = 0;
            dest_smooth[x][50] = 0;
            dest_smooth[x][51] = 0;
        }

        if ((i%2)==0)
        {
            for (int x=0;x!=16;x++)
                for (int y=0;y!=16;y++)
                    dest_smooth[x][y] = (x+y)%2;
        }
#endif

        copy_image( previous_smooth, dest_smooth );

        if (dump)
        {
            sprintf( buffer, "out-%06d.pgm", i );
            write_image( buffer, dest_smooth );
        }

        if (cover_index!=-1 && i>=cover_index && i<cover_index+24)
        {
            if (debug)
                std::clog << "COVER AT " << i << "\n";

            sprintf( buffer, "cover-%06d.pgm", i-cover_index );
            write_image( buffer, dest_smooth );
        }

        encode( out, dest_smooth );

        frame_count++;
    }

    return frame_count;
}

typedef enum { kVideo = 0 } eStreamType;
typedef enum { kSilent = 0 } eStreamSubType;
typedef enum { kSD = 0, kHD = 1 } eResolution;

//  Confused class that performs the encoding
//  (Confused because it keeps a pointer to a shared data structure to be able to
//  write the header. All this is pretty badly designed, if you ask me)
class stream_encoder
{
    std::vector<u_int8_t> &vec_;
    int index_;

public:
    stream_encoder( std::vector<u_int8_t> &vec, eStreamType type, eStreamSubType sub_type, eResolution resolution, float fps ) :
        vec_{ vec }
    {
    //  short stream type
        vec_.push_back( 0x00 );
        vec_.push_back( type );

    //  short tream subtype
        vec_.push_back( 0x00 );
        vec_.push_back( sub_type );

    //  shortx2 resolution: 512x342 / 256x171
        int w=0,h=0;
        if (resolution==kSD)
        {
            w = 256;
            h = 171;
        }
        if (resolution==kHD)
        {
            w = 512;
            h = 342;
        }
        vec_.push_back( w/256 );
        vec_.push_back( w%256 );
        vec_.push_back( h/256 );
        vec_.push_back( h%256 );

        // fixed short.short fps: frames per second, in 65536th of frames
        int fps0 = fps;
        int fps1 = ((int)(fps*65536))%65536;
        vec_.push_back( fps0/256 );
        vec_.push_back( fps0%256 );
        vec_.push_back( fps1/256 );
        vec_.push_back( fps1%256 );

        index_ = vec_.size();

        //  space for frame count
        vec_.push_back( 0xff );
        vec_.push_back( 0xff );
        vec_.push_back( 0xff );
        vec_.push_back( 0xff );

        //  long offset: offset to the flim from begining of file.
        vec_.push_back( 0xff );
        vec_.push_back( 0xff );
        vec_.push_back( 0xff );
        vec_.push_back( 0xff );

        //  long length: length of the stream (width * height * frame count / 8 for movies)
        vec_.push_back( 0xff );
        vec_.push_back( 0xff );
        vec_.push_back( 0xff );
        vec_.push_back( 0xff );
    }

    void add_encoded_data( int frame_count, const std::vector<u_int8_t> &data )
    {
        long offset = vec_.size();

        vec_.insert( vec_.end(), data.begin(), data.end() );

        long len = data.size();

    // encode<256,171>( std::back_inserter(res), in_arg, from_index, to_index );

        vec_[index_+ 0] = frame_count>>24;
        vec_[index_+ 1] = frame_count>>16;
        vec_[index_+ 2] = frame_count>>8;
        vec_[index_+ 3] = frame_count;

        vec_[index_+ 4] = offset>>24;
        vec_[index_+ 5] = offset>>16;
        vec_[index_+ 6] = offset>>8;
        vec_[index_+ 7] = offset;

        vec_[index_+ 8] = len>>24;
        vec_[index_+ 9] = len>>16;
        vec_[index_+10] = len>>8;
        vec_[index_+11] = len;
    }
};

//  The main function, does all the work
//  flimmaker [-g] --in <%d.pgm> --from <index> --to <index> --cover <index> --out <file>
int main( int argc, char **argv )
{
    std::string in_arg = "movie-%06d.pgm";
    std::string out_arg = "out.flim";
    int from_index = 1;
    int to_index = std::numeric_limits<int>::max();
    int cover_index = -1;

    argc--;
    argv++;

    while (argc)
    {
        if (!strcmp(*argv,"-g"))
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

    if (cover_index==-1)
        cover_index = from_index+(to_index-from_index)/3;

//    pack_test();

    mac_image neutral;
    mac_image save;

    fill( neutral );

    mac_image previous_smooth;

    mac_image previous_neutral;
    fill( previous_smooth, 0 );
    fill( previous_neutral, 0 );

    mac_image source;
    mac_image previous_source;

    mac_image dest_smooth;
    mac_image dest_neutral;

    mac_image diff_smooth;
    mac_image diff_neutral;


    std::vector<u_int8_t> res;

    //  Write header
    res.push_back( 'f' );
    res.push_back( 'l' );
    res.push_back( 'i' );
    res.push_back( 'm' );
    res.push_back( '\r' );
    res.push_back( '\n' );
    res.push_back( '0' );
    res.push_back( '1' );
    res.push_back( '\r' );
    res.push_back( '\n' );

//  short Stream count
    res.push_back( 0x00 );
    res.push_back( 0x04 );

    stream_encoder se0( res, kVideo, kSilent, kSD, 12 );
    stream_encoder se1( res, kVideo, kSilent, kSD, 24 );
    stream_encoder se2( res, kVideo, kSilent, kHD, 12 );
    stream_encoder se3( res, kVideo, kSilent, kHD, 24 );

    std::vector<u_int8_t> data;
    int count;
        
    data.clear();
    count = encode<256,171>( std::back_inserter(data), in_arg, from_index, to_index, 2 );
    se0.add_encoded_data( count, data );
#ifdef STAMP
    sStream++;
#endif

    data.clear();
    count = encode<256,171>( std::back_inserter(data), in_arg, from_index, to_index, 1 );
    se1.add_encoded_data( count, data );
#ifdef STAMP
    sStream++;
#endif

    data.clear();
    count = encode<512,342>( std::back_inserter(data), in_arg, from_index, to_index, 2 );
    se2.add_encoded_data( count, data );
#ifdef STAMP
    sStream++;
#endif

    data.clear();
    count = encode<512,342>( std::back_inserter(data), in_arg, from_index, to_index, 1, true, cover_index );
    se3.add_encoded_data( count, data );
#ifdef STAMP
    sStream++;
#endif

    FILE *movie_file = fopen( out_arg.c_str(), "wb" );
    fwrite( res.data(), res.size(), 1, movie_file );
    fclose( movie_file );

    return EXIT_SUCCESS;
}
