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
#include "flimcompressor.hpp"

using namespace std::string_literals;

//  True if the global '-g' option was set
bool debug = false;

//  If defined, we add a "stamp" to each stream, to know where it is coming from
#define noSTAMP

#ifdef STAMP
static int sStream = 0;
#endif



#include "image.hpp"


// static float g_stability = 0.30;
static float g_max_stability = 1.00;





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


#if 0
/**
 * A pack is an encoded series of transformation from a framebuffer into another one
 */
template <int W, ing H>
class pack
{
    std::vector<u_int8_t> data_;

    public:
        pack( const framebuffer<W,H> &x )
            : data_{
                x.packed_data( framebuffer<W,H>::kPackZeroes ),
                x.packed_size( framebuffer<W,H>::kPackZeroes )
            }
        {
        }

        framebuffer<W,H> apply( const framebuffer<W,H> &source ) const
        {
            auto res = source;

            auto s = data_.data();  //  source
            auto d = res.data();    //  destination

            while (1)
            {
                auto n = *s++;
                if (n==0)
                    break;
                if (n>0)
                {
                    while (n--)
                        *d++ ^= *s++;
                }
                else
                    d -= n;
            }

            return res;
        }
};





/**
 * This compresses a frame
 * We try to increment compression incresing stability
 * If this fails, we can collapse the 2 ticks
*/
template <int W, int H>
class FrameCompressor
{
public:
    FrameCompressor(
        const framebuffer<W,H> &before,
        const image<W,H> &before_img_,
        const image<W,H> &img_,
        const image<W,H> &next_img_,
        size_t ticks_,
        size_t next_ticks
    )
    {
        img = img_;
        before_img = before_img_;

        quantize( quantized_img, img, before_img, stability );
        framebuffer<W,H> fb{ quantized_img };


    }
};
#endif

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

/*
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
*/

/*
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

    double cur_tick = 0;
    size_t last_integral_tick = 0;

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

        std::vector<u_int8_t> block_data;
        auto p = std::back_inserter( block_data );

        write4( p, 0x41424344 );    //  'ABCD'

        auto lt = last_integral_tick;

        for (auto &video_frame:packed_frames)
        {
            std::vector<u_int8_t> frame_data;
            auto q = std::back_inserter( frame_data );

            write4( q, 0x45464748 );    //  'EFGH'

                //  Where we should be when we will have displayed this frame
            cur_tick += 60.0/25.0;

                //  How much we can really move
            size_t delta_ticks = (cur_tick+0.5) - last_integral_tick;
            
            assert( delta_ticks!=0 );

            last_integral_tick += delta_ticks;

                //  This frame will have that amount of ticks
            write2( q, delta_ticks );

                //  Let's write the sound
                                    //  FFSYnth header
            write2( q, 0 );         //  ffMode
            write4( q, 65536 );     //  Fixed(1,1)

                                    //  Samples
            for (int t=0;t!=delta_ticks;t++)
                for (int i=0;i!=370;i++)
                    write1( q, *g_soundptr++ );

                //  Current encoding: byte-level xor
            // write2( p, 0x0001 );
            //  already present in frame encoded data

            fprintf( stderr, "%5ld/%1ld ", video_frame.size(), delta_ticks );

            // fprintf( stderr, "\n %d %d %d %d\n", video_frame[0], video_frame[1], video_frame[2], video_frame[3] );
            for (auto b:video_frame)
            {
                write1( q, b );
            }

            while ((frame_data.size()%4)!=0)
                write1( q, 0xff );

            write4( p, frame_data.size()+4 );
            std::copy( std::begin( frame_data ), std::end( frame_data ), p );
        }

        while ((block_data.size()%4)!=0)
            write1( p, 0xff );

        write4( out, block_data.size()+4 );
        std::copy( std::begin( block_data ), std::end( block_data ), out );

        fprintf( stderr, " %ld ticks\n", last_integral_tick-lt );

        if (failed)
            break;
    }

    write4( out, 0 );

    return frame_count;
}
*/

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
    size_t byterate = 6000;
    double fps = 24.0;
    size_t block_size = 20;
    double stability = 0.3;

    packz32_test();
    packz32opt_test();

    argc--;
    argv++;

    while (argc)
    {
        // if (!strcmp(*argv,"--compression-target"))
        // {
        //     argc--;
        //     argv++;
        //     compression_target = atof(*argv)/100.0;
        //     argc--;
        //     argv++;
        // }
        // else
        if (!strcmp(*argv,"--byterate"))
        {
            argc--;
            argv++;
            byterate = atoi(*argv);
            argc--;
            argv++;
        }
        else if (!strcmp(*argv,"--fps"))
        {
            argc--;
            argv++;
            fps = atof(*argv);
            argc--;
            argv++;
        }
        else if (!strcmp(*argv,"--block-size"))
        {
            argc--;
            argv++;
            block_size = atoi(*argv);
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
        // else if (!strcmp(*argv,"--cover"))
        // {
        //     argc--;
        //     argv++;
        //     cover_index = atoi(*argv);
        //     argc--;
        //     argv++;
        // }
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
            stability = atof( *argv );
            argc--;
            argv++;
        }
        // else if (!strcmp(*argv,"--max-stability"))
        // {
        //     argc--;
        //     argv++;
        //     g_max_stability = atof( *argv );
        //     argc--;
        //     argv++;
        // }
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

    auto encoder = flimencoder<512,342>{ in_arg, audio_arg };
    encoder.set_byterate( byterate );
    encoder.set_fps( fps );
    encoder.set_block_size( block_size );
    encoder.set_stability( stability );
    encoder.make_flim( out_arg, from_index, to_index );

    return EXIT_SUCCESS;
}
