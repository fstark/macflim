#ifndef FILM_COMPRESSOR_INCLUDED__
#define FILM_COMPRESSOR_INCLUDED__

#include "image.hpp"
#include <vector>
#include <bitset>
#include <algorithm>

#define VERBOSE

using namespace std::string_literals;
    
template <int W, int H>
class compressor
{
    static const size_t size = framebuffer<W,H>::lsize;

    std::array<uint32_t,size> current_data_;    //  The data present on screen (for optimisation purposes)
    std::array<uint32_t,size> target_data_;     //  The data we are trying to converge to
    std::array<size_t,size> delta_;                //  0: it is sync'ed

    size_t xcountbits( uint32_t v ) const
    {
        std::bitset<32> b{ v };
        return b.count();
    }

    size_t countbits( uint32_t v, size_t from, size_t to ) const
    {
        size_t res = 0;
        for (int i=from;i!=to;i++)
            res += !!(v&(1<<i));
        return res;
    }

    size_t distancebits( uint32_t v0, uint32_t v1, size_t from, size_t to ) const
    {
        auto c0 = countbits( v0, from, to );
        auto c1 = countbits( v1, from, to );
        return std::abs( (int)c0-(int)c1 );
    }

    size_t distance( uint32_t v0, uint32_t v1, size_t width ) const
    {
        size_t res = 0;
        for (size_t i=0;i!=32;i+=width)
            res += distancebits( v0, v1, i, i+width );
        return res;
    }

    size_t distance( uint32_t v0, u_int32_t v1 ) const
    {
        return xcountbits( v0^v1 ) + distance( v0, v1, 2 ) + distance( v0, v1, 4 ) + distance( v0, v1, 8 ) + distance( v0, v1, 16 ) + distance( v0, v1, 32 );
        // return xcountbits( v0^v1 );
    }

    int frame = 0;

public:
    compressor()
    {
        for (int i=0;i!=size;i++)
        {
            current_data_[i] = 0xffffffff;
            target_data_[i] = 0xffffffff;
            delta_[i] = 0;
        }
    }

    double quality() const
    {
        int b = 0;
        for (int i=0;i!=size;i++)
            b += xcountbits( current_data_[i]^target_data_[i] );
        return 1-b/(double)(W*H);
    }

    // std::array<bool,size> over( int age ) const
    // {
    //     std::array<bool,size> need;
    //     for (int i=0;i!=size;i++)
    //         need[i] = diff_[i]>=age;

    //     auto p = std::begin( need );

    //     for (int y=0;y!=H;y++)
    //         for (int x=0;x!=W/32;x++)
    //         {
    //             // if ((((y*3)/342))!=(frame%3))
    //             // if ((y%3)!=(frame%3))
    //             //     *p = false;
    //             p++;
    //         }
        
    //     return need;
    // }

    // std::vector<uint32_t> compress_age( int age, size_t max_size ) const
    // {
    //     return packz32opt( target_data_, over(age), max_size );
    // }

    // std::vector<uint32_t> compress_age( int age ) const
    // {
    //     packzmap<size> foo;
    //     packz32opt( target_data_, foo );
    //     return packz32opt( target_data_, over(age) );
    // }

//     int best_age( size_t max_size ) const
//     {
// #ifdef VERBOSE
//         std::clog << "BEST AGE FOR " << max_size << "\n  ";
// #endif
//         auto mx = *std::max_element( std::begin(diff_), std::end(diff_) );
//         if (mx==0)
//             mx = 1;
// #ifdef VERBOSE
//         for (int i=0;i<=mx;i++)
//             std::clog << i << ":" << std::count( std::begin(diff_), std::end(diff_), i ) <<"  ";
//         std::clog << "\n  ";
// #endif
//         for (int s=2;s<=mx;s++)
//         {
//             auto res = compress_age( s );
// #ifdef VERBOSE
//             std::clog << s << " => " << res.size() << " ";
// #endif
//             if (res.size()<=max_size)
//             {
// #ifdef VERBOSE
//                 std::clog << " (done with " << s-1 << ")\n";
// #endif
//                 return s-1;
//             }
//         }
//         //  This is a problem: we can't compress well enough -- let's limit the compression
        
// #ifdef VERBOSE
//         std::clog << " done with max" << "\n";
// #endif
//         return mx;
//     }


    std::vector<uint32_t> compress( size_t max_size )
    {
        packzmap<size> packmap;

        auto mx = *std::max_element( std::begin(delta_), std::end(delta_) );

        std::vector<std::vector<size_t>> deltas;
        deltas.resize( mx+1 );

        for (int i=0;i!=size;i++)
            if (delta_[i])
                deltas[delta_[i]].push_back( i );

        for (int i=deltas.size()-1;i!=0;i--)
            for (auto ix:deltas[i])
                if (packmap.set(ix)>=max_size)
                {
                    packmap.clear(ix);
                    break;
                }

        return packz32opt( target_data_, packmap );
    }

    void dump_histogram( const std::string msg )
    {
        auto mx = *std::max_element( std::begin(delta_), std::end(delta_) );
        std::clog << msg << " ";
        for (int i=0;i<=mx;i++)
            fprintf( stderr, "%3d:%5ld ", i, std::count( std::begin(delta_), std::end(delta_), i ) );
            // std::clog << i << ":" << std::count( std::begin(delta_), std::end(delta_), i ) <<"  ";
        std::clog << "\n";
    }

        /// Progress toward the target image, using less than max_size bytes
    std::vector<uint32_t> next_tick( size_t max_size )
    {
        frame++;

        // dump_histogram( "BEFORE" );

        auto res = compress( max_size );

        size_t index = 0;
        auto s = std::begin(res);

        while (*s)
        {
            uint32_t header = *s++;
            index += header>>16;
            int count = header & 0xffff;
            while (count--)
            {
                current_data_[index] = *s++;
                delta_[index] = 0;
                index++;
            }
        };

        // dump_histogram( "AFTER " );

        //  logs current image
        {
            static int img = 1;
            char buffer[1024];
            sprintf( buffer, "out-%06d.pgm", img++ );
            framebuffer<W,H> fb{current_data_};
            auto logimg = fb.as_image();
            write_image( buffer, logimg );
        }
        return res;
    }

        /// Sets the new target image
    void set_target_image( const framebuffer<W,H> &image )
    {
        auto new_data = image.raw32();
        for (int i=0;i!=size;i++)
        {
            target_data_[i] = new_data[i];
            if (current_data_[i]==target_data_[i])
                    //  Data is identical, we don't care about updating this
                delta_[i] = 0;
            else
            {
                    //  Let's increase the importance of updating this
                // delta_[i] += countbits( target_data_[i] ^ current_data_[i] );
                delta_[i] = distance( target_data_[i], current_data_[i] );
                // printf( "DIFF %x %x = %d\n", target_data_[i], current_data_[i], diff_[i] );
            }
        }
    }
};

template <int W, int H>
class flimcompressor
{
public:
    struct frame
    {
        std::array<uint8_t,370> audio;
        std::vector<uint32_t> video;
    };

private:
    const std::vector<image<W,H>> &images_;
    const std::vector<uint8_t> &audio_;
    const double fps_;

    std::vector<frame> frames_;

public:
    flimcompressor( const std::vector<image<W,H>> &images, const std::vector<uint8_t> &audio, double fps ) : images_{images}, audio_{audio}, fps_{fps} {}

    std::vector<std::uint32_t> header()
    {
        std::vector<std::uint32_t> header;

        return header;
    }        

    const std::vector<frame> &get_frames() const { return frames_; }

    void compress( double stability, size_t byterate )
    {
        std::vector<std::uint32_t> data = header();

        compressor<W,H> c;

        image<W,H> previous;
        fill( previous, 0 );

        int in_fr=0;
        int out_fr=0;
        double current_time = 0;
        double theorical_time = 0;

        size_t fail1=0;
        size_t fail2=0;

        double total_q = 0;

        for (auto &source_image:images_)
        {
            image<W,H> dest;

            in_fr++;
            theorical_time = in_fr*(1/fps_);

            quantize( dest, source_image, previous, stability );
            previous = dest;

            round_corners( dest );
            framebuffer<W,H> fb{dest};

            c.set_target_image( fb );

            while (theorical_time>current_time)
            {
                frame f;
//                std::clog << "time: " << current_time << " " << theorical_time << "\n";
                std::copy( std::begin(audio_)+370*out_fr, std::begin(audio_)+370*out_fr+370, std::begin(f.audio) );
                f.video = c.next_tick( byterate );
                frames_.push_back( f );
                out_fr++;
                // fprintf( stderr, "# %4d/%4d (%5.3f) %5ld bytes\n", in_fr, out_fr, current_time, data.size()*sizeof(data[0]) );
                current_time += 1/60.0;
            }

            auto q = c.quality();
            total_q += q;
            if (q!=1)
            {
                fprintf( stderr, "# %4d/%4d (%5.3f) \u001b[%sm%05.3f%%\u001b[0m\n", in_fr, out_fr, current_time, q<.9?"91":"0", q*100 );
            }
            if (q<0.9)
                fail2++;
            if (q<1)
                fail1++;
        }

        fprintf( stderr, "FAIL %ld/%ld avg %7.5f on %d\n", fail2, fail1, total_q/in_fr*100, in_fr );
    }
};

#include <string>

template <int W, int H>
class flimencoder
{
    const std::string in_;
    const std::string audio_;

    std::vector<image<W,H>> images_;
    std::vector<uint8_t> audio_samples_;

    size_t byterate_ = 2000;
    double fps_ = 24.0;

    size_t block_size_ = 20;

    double stability_ = 0.3;


    size_t frame_from_image( size_t n ) const
    {
        return (n-1)/fps_*60;
    }

    //  Read all images from disk
    void read_images( size_t from, size_t to )
    {
        std::clog << "READ IMAGES ";

        static char symb[] = "123456789.";

        for (int i=from;i!=to+1;i++)
        {
            char buffer[1024];
            sprintf( buffer, in_.c_str(), i );

            image<W,H> img;

            if (!read_image( img, buffer ))
                return;
            images_.push_back( img );

            std::clog << symb[i%(sizeof(symb)-1)];
            if ((i%(sizeof(symb)-1))!=(sizeof(symb)-2))
                std::clog << (char)0x8;
            std::clog << std::flush;
        }
        std::clog << "\n";
        std::clog << "VIDEO: READ " << images_.size() << " images\n";
    }

    //  Read all audio from disk
    void read_audio( size_t from, size_t to )
    {
        long audio_start = frame_from_image(from)*370;    //  Images are one-based
        long audio_end = frame_from_image(to+1)*370;      //  Last image is included
        long audio_real_end;
        long audio_size;
        FILE *f = fopen( audio_.c_str(), "rb" );
        if (f)
        {
            fseek( f, 0L, SEEK_END );
            audio_real_end = std::min( ftell( f ), audio_end );
            
            audio_size = audio_real_end-audio_start;
            if (audio_size>0)
            {
                audio_samples_.resize( audio_size );
                fseek( f, audio_start, SEEK_SET );
                if (fread( audio_samples_.data(), audio_size, 1, f )!=1)
                    std::cerr << "**** WARNING: CANNOT READ AUDIO FILE [" << audio_ << "]\n";
            }
            else
                std::clog << "#### INFO : NO AUDIO INCLUDED\n";
            fclose( f );
        }
        else
            std::cerr << "**** ERROR: CANNOT OPEN AUDIO FILE [" << audio_ << "]\n";
        std::clog << "AUDIO: READ " << audio_size << " bytes from offset " << audio_start << "\n";
    }

    void fix()
    {
        //  TODO: make sure images and sound size matches

        // std::clog << images_.size() << "\n";
        // std::clog << frame_from_image(images_.size()+1) << "\n";
        // std::clog << audio_samples_.size() << "\n";
        assert( frame_from_image(images_.size()+1)*370 == audio_samples_.size() );
    }

public:
    flimencoder( const std::string &in, const std::string &audio ) : in_{in}, audio_{audio} {}

    void set_byterate( size_t byterate ) { byterate_ = byterate; }
    void set_fps( size_t fps ) { fps_ = fps; }
    void set_block_size( size_t block_size ) { block_size_ = block_size; }
    void set_stability( double stability ) { stability_ = stability; }

    //  Encode all the blocks
    void make_flim( const std::string flim_pathname, size_t from, size_t to )
    {  
        read_images( from, to );
        read_audio( from, to );

        fix();

        flimcompressor<W,H> fc{ images_, audio_samples_, fps_ };

        fc.compress( stability_, byterate_/4 );

        auto frames = fc.get_frames();
        auto frame_count = frames.size();

        std::vector<uint8_t> movie;

        int block_count = frame_count/block_size_;
        for (int i=0;i!=block_count;i++)
        {
            int block_size = block_size_;
            if (i==block_count-1)
                block_size = frame_count%block_size_;

            std::vector<uint8_t> block;
            auto out = std::back_inserter(block);

            write4( out, 0x464C494D );
            write2( out, block_size );
            write2( out, 0xffff );
            for (int j=0;j!=block_size;j++)
            {
                const typename flimcompressor<W,H>::frame &f = frames[i*block_size_+j];
                write2( out, 0 );           //  ffMode
                write4( out, 65536 );       //  rate
                write( out, f.audio );
                write2( out, f.video.size()*4 );
                write( out, f.video );
            }

            auto out_movie = std::back_inserter( movie );
            write4( out_movie, block.size()+4 );
            write( out_movie, block );
        }

        FILE *movie_file = fopen( flim_pathname.c_str(), "wb" );
        fwrite( movie.data(), movie.size(), 1, movie_file );
        fclose( movie_file );
    }
};

#endif
