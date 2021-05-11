#ifndef FILM_COMPRESSOR_INCLUDED__
#define FILM_COMPRESSOR_INCLUDED__

#include "image.hpp"
#include <vector>
#include <bitset>
#include <algorithm>

#define VERBOSE

using namespace std::string_literals;

class compressor
{
    size_t W_;
    size_t H_;
    // static const size_t data_size = W_*H_/8/4;

    std::vector<uint32_t> current_data_;    //  The data present on screen (for optimisation purposes)
    std::vector<uint32_t> target_data_;     //  The data we are trying to converge to
    std::vector<size_t> delta_;                //  0: it is sync'ed

    size_t  get_uint32_size() const { return W_*H_/8/4; }

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
    compressor( size_t W, size_t H) : current_data_(W*H/8/4), target_data_(W*H/8/4), delta_(W*H/8/4), W_{W}, H_{H}
    {
        std::fill( std::begin(current_data_), std::end(current_data_), 0xffffffff );
        std::fill( std::begin(target_data_), std::end(target_data_), 0xffffffff );
        std::fill( std::begin(delta_), std::end(delta_), 0x00000000 );
    }

    framebuffer get_current_framebuffer() const { return framebuffer(current_data_,W_,H_); }
    framebuffer get_target_framebuffer() const { return framebuffer(target_data_,W_,H_); }

    double quality() const
    {
        int b = 0;
        for (int i=0;i!=get_uint32_size();i++)
            b += xcountbits( current_data_[i]^target_data_[i] );
        return 1-b/(double)(W_*H_);
    }

    std::vector<uint32_t> compress( size_t max_size )
    {
        packzmap packmap{ get_uint32_size() };

        auto mx = *std::max_element( std::begin(delta_), std::end(delta_) );

        std::vector<std::vector<size_t>> deltas;
        deltas.resize( mx+1 );

        for (int i=0;i!=get_uint32_size();i++)
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

        auto s = std::begin(res);

/*        res.resize(0);
        res.push_back( 0x000f0004 );
        res.push_back( 0x0fffffff );
        res.push_back( 0x0fffffff );
        res.push_back( 0x00ffffff );
        res.push_back( 0x00ffffff );
        res.push_back( 0x000fffff );
        res.push_back( 0x000fffff );
        res.push_back( 0x0000ffff );
        res.push_back( 0x0000ffff );
        res.push_back( 0x00000fff );
        res.push_back( 0x00000fff );
        res.push_back( 0x000000ff );
        res.push_back( 0x000000ff );
        res.push_back( 0x0000000f );
        res.push_back( 0x0000000f );
        res.push_back( 0x00000000 );
        res.push_back( 0x00000000 );

        res.push_back( 0x00070008 );
        res.push_back( 0x0fffffff );
        res.push_back( 0x00ffffff );
        res.push_back( 0x000fffff );
        res.push_back( 0x0000ffff );
        res.push_back( 0x00000fff );
        res.push_back( 0x000000ff );
        res.push_back( 0x0000000f );
        res.push_back( 0x00000000 );

        res.push_back( 0x0007020c );
        res.push_back( 0x0fffffff );
        res.push_back( 0x00ffffff );
        res.push_back( 0x000fffff );
        res.push_back( 0x0000ffff );
        res.push_back( 0x00000fff );
        res.push_back( 0x000000ff );
        res.push_back( 0x0000000f );
        res.push_back( 0x00000000 );

        res.push_back( 0x00000000 );
*/
        while (*s)
        {
            uint32_t header = *s++;
            uint32_t offset = ((header&0xffff)-4);

            uint32_t scr_x = offset%64;
            uint32_t scr_y = offset/64;

            scr_x /= 4;

            offset = scr_x * 342 + scr_y;

            int count = (header>>16)+1;

            while (count--)
            {
                current_data_[offset] = *s++;
                delta_[offset] = 0;
                offset++;
            }
        };

        // dump_histogram( "AFTER " );

        //  logs current image
        {
            static int img = 1;
            char buffer[1024];
            sprintf( buffer, "out-%06d.pgm", img );
            framebuffer fb{current_data_, W_, H_};
            auto logimg = fb.as_image();
            write_image( buffer, logimg );

            img++;
        }
        return res;
    }

        /// Sets the new target image
    void set_target_image( const framebuffer &image )
    {
        assert( image.W()==W_ && image.H()==H_ );
        auto new_data = image.raw32();
        for (int i=0;i!=get_uint32_size();i++)
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

inline size_t ticks_from_frame( size_t n, double fps ) { return n/fps*60; }

class flimcompressor
{
public:
    struct frame
    {
        frame( size_t W, size_t H ) : result{ W, H } {}

        size_t ticks;
        std::vector<uint8_t> audio;
        std::vector<uint32_t> video;
        framebuffer result;

        size_t get_size() { return audio.size()+video.size()*4; }
    };

private:
    const std::vector<image> &images_;
    const std::vector<uint8_t> &audio_;
    const double fps_;

    std::vector<frame> frames_;

    size_t W_;
    size_t H_;

public:
    flimcompressor( size_t W, size_t H, const std::vector<image> &images, const std::vector<uint8_t> &audio, double fps ) : W_{W}, H_{H}, images_{images}, audio_{audio}, fps_{fps} {}

    std::vector<std::uint32_t> header()
    {
        std::vector<std::uint32_t> header;

        return header;
    }        

    const std::vector<frame> &get_frames() const { return frames_; }

    void compress( double stability, size_t byterate, bool group, const std::string &filters )
    {
        std::vector<std::uint32_t> data = header();

        compressor c{W_,H_};

        image previous( W_, H_ );
        fill( previous, 0 );

        int in_fr=0;

        size_t current_tick = 0;

        size_t fail1=0;
        size_t fail2=0;

        double total_q = 0;

            //  The audio ptr
        auto audio = std::begin( audio_ );

        for (auto &source_image:images_)
        {
            image dest( W_, H_ );

            image img = filter( source_image, filters.c_str() );

            quantize( dest, img, previous, stability );
            previous = dest;
            //  dest = filter( dest, "gsc" );
            
            round_corners( dest );
            framebuffer fb{ dest };
            c.set_target_image( fb );

                //  Let's see how many ticks we have to display this image
            in_fr++;
            size_t next_tick = ticks_from_frame( in_fr, fps_ );
            size_t ticks = next_tick-current_tick;
            assert( ticks>0 );


            size_t local_ticks = 1;

            if (group)
                local_ticks = ticks;

            for (int i=0;i!=ticks;i+=local_ticks)
            {

                    //  Build the frame
                frame f{ W_, H_ };

                f.ticks = local_ticks;

                std::copy( audio, audio+370*local_ticks, std::back_inserter(f.audio) );
                audio += 370*local_ticks;
                assert( audio<=std::end(audio_) );

                    //  What is the video budget?
                size_t video_budget = byterate*local_ticks;

                    //  Encode withing that budget
                f.video = c.next_tick( video_budget );

                    //  Add the current frame buffer
                f.result = c.get_current_framebuffer();

                frames_.push_back( f );
            }


            auto q = c.quality();
            total_q += q;
            if (q!=1)
            {
                fprintf( stderr, "# %4d (%5.3f) \u001b[%sm%05.3f%%\u001b[0m\n", in_fr, current_tick/60.0, q<.9?"91":"0", q*100 );
            }
            if (q<0.9)
                fail2++;
            if (q<1)
                fail1++;

            current_tick = next_tick;
        }

        fprintf( stderr, "Total input frames: %d. Rendered at 80%%: %ld. Rendered at 90%%: %ld. Average rendering %7.5f%%.\n", in_fr, fail2, fail1, total_q/in_fr*100 );
    }
};

#include <string>

class flimencoder
{
    size_t W_;
    size_t H_;

    const std::string in_;
    const std::string audio_;

    std::vector<image> images_;
    std::vector<uint8_t> audio_samples_;

    size_t byterate_ = 2000;
    double fps_ = 24.0;

    size_t buffer_size_ = 300000;

    double stability_ = 0.3;

    bool half_rate_ = false;
    bool group_ = true;

    std::string comment_;

    std::string filters_;

    size_t cover_begin_;        /// Begin index of cover image
    size_t cover_end_;          /// End index of cover image 

    size_t frame_from_image( size_t n ) const
    {
        return ticks_from_frame( n-1, fps_ );
    }

    //  Read all images from disk
    void read_images( size_t from, size_t to, bool half_rate=false )
    {
        std::clog << "READ IMAGES ";

        static char symb[] = "123456789.";

        bool skip = false;

        for (int i=from;i!=to+1;i++)
        {
            if (half_rate)
            {
                if (skip)
                {
                    skip = false;
                    continue;
                }
                skip = true;
            }

            char buffer[1024];
            sprintf( buffer, in_.c_str(), i );

            image img( W_, H_ );

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
    void read_audio( size_t from, size_t count )
    {
        long audio_start = frame_from_image(from)*370;    //  Images are one-based
        long audio_end = frame_from_image(from+count)*370;      //  Last image is included?
        long audio_size = audio_end-audio_start;

        FILE *f = fopen( audio_.c_str(), "rb" );
        if (f)
        {
            audio_samples_.resize( audio_size );
            for (auto &v:audio_samples_)
                v = 0x80;

            fseek( f, audio_start, SEEK_SET );
            auto res = fread( audio_samples_.data(), 1, audio_size, f );
            if (res!=audio_size)
                std::clog << "AUDIO: added " << audio_size-res << " bytes of silence\n";
            fclose( f );
        }
        else
            std::cerr << "**** ERROR: CANNOT OPEN AUDIO FILE [" << audio_ << "]\n";
        std::clog << "AUDIO: READ " << audio_size << " bytes from offset " << audio_start << "\n";

        auto min_sample = *std::min( std::begin(audio_samples_), std::end(audio_samples_) );
        auto max_sample = *std::max( std::begin(audio_samples_), std::end(audio_samples_) );
        std::clog << "MIN= " << (int)min_sample << " MAX= " << (int)max_sample << "\n";
    }

    void fix()
    {
        //  TODO: make sure images and sound size matches

        std::clog << "**** fps                : " << fps_ << "\n";

        std::clog << "**** # of video images  : " << images_.size() << "\n";
        std::clog << "**** # of video frames  : " << frame_from_image(images_.size()+1) << "\n";
        std::clog << "**** # of audio samples : " << audio_samples_.size() << "\n";
        std::clog << "**** # of audio frames  : " << audio_samples_.size()/370.0 << "\n";

        // std::clog << images_.size() << "\n";
        // std::clog << frame_from_image(images_.size()+1) << "\n";
        // std::clog << audio_samples_.size() << "\n";
        if (frame_from_image(images_.size()+1)*370 != audio_samples_.size())
        {
            std::cerr << "**** ERROR : " << frame_from_image(images_.size()+1)*370 << "!=" << audio_samples_.size() << "\n";
            // assert( false );
        }
    }

public:
    flimencoder( size_t W, size_t H, const std::string &in, const std::string &audio ) : W_{W}, H_{H}, in_{in}, audio_{audio} {}

    void set_byterate( size_t byterate ) { byterate_ = byterate; }
    void set_fps( double fps ) { fps_ = fps; }
    void set_buffer_size( size_t buffer_size ) { buffer_size_ = buffer_size; }
    void set_stability( double stability ) { stability_ = stability; }
    void set_half_rate( bool half_rate ) { half_rate_ = half_rate; }
    void set_group( bool group ) { group_ = group; }
    void set_comment( const std::string &comment ) { comment_ = comment; }
    void set_filters( const std::string &filters ) { filters_ = filters; }
    void set_cover( size_t cover_begin, size_t cover_end ) { cover_begin_ = cover_begin; cover_end_ = cover_end; }

    //  Encode all the blocks
    void make_flim( const std::string flim_pathname, size_t from, size_t to )
    {  
        if (half_rate_)
        {
            fps_ /= 2;
        }

        read_images( from, to, half_rate_ );
        read_audio( from, images_.size() );

        fix();

        flimcompressor fc{ W_, H_, images_, audio_samples_, fps_ };

        fc.compress( stability_, byterate_/4, group_, filters_ );

        auto frames = fc.get_frames();

        std::vector<uint8_t> movie;
        auto out_movie = std::back_inserter( movie );

        auto block_first_frame = std::begin(frames);

        auto current_frame = block_first_frame;
        while(current_frame!=std::end(frames))
        {
            std::vector<u_int8_t> block_content;
            auto block_ptr = std::back_inserter( block_content );
            size_t current_block_size = 0;

            while (current_block_size+current_frame->get_size()<buffer_size_ && current_frame!=std::end(frames))
            {
                write2( block_ptr, current_frame->ticks*370+8 );           //  size of sound + header + size itself
                write2( block_ptr, 0 );                       //  ffMode
                write4( block_ptr, 65536 );                   //  rate
                write( block_ptr, current_frame->audio );
                write2( block_ptr, current_frame->video.size()*4+2 );
                write( block_ptr, current_frame->video );

                current_block_size += current_frame->get_size();
                current_frame++;
            }
            write4( out_movie, block_content.size()+4 /* size */+4 /* 'FLIM' */+2/* frames */ );
            write4( out_movie, 0x464C494D );
            write2( out_movie, current_frame-block_first_frame );

            write( out_movie, block_content );
            block_first_frame = current_frame;
        }
        FILE *movie_file = fopen( flim_pathname.c_str(), "wb" );

        char buffer[1024];
        std::fill( std::begin(buffer), std::end(buffer), 0 );
        strcpy( buffer, comment_.c_str() );
        fwrite( buffer, 1024, 1, movie_file );

            //  Adds end mark
        movie.push_back( 0x00 );
        movie.push_back( 0x00 );
        movie.push_back( 0x00 );
        movie.push_back( 0x00 );

        fwrite( movie.data(), movie.size(), 1, movie_file );
        fclose( movie_file );

        //  Generating the cover
        for (size_t i=cover_begin_;i<=cover_end_;i++)
        {
            if (i<frames.size())
            {
                char buffer[1024];
                std::clog << "COVER " << i << "\n";
                sprintf( buffer, "cover-%06ld.pgm", i-cover_begin_+1 );
                auto logimg = frames[i].result.as_image();
                write_image( buffer, logimg );
            }
        }


    /*
        int frames_per_buffer = buffer_size_/(byterate_+370+70); //overhhead max 

        for (int i=0;i<frame_count;i+=frames_per_buffer)
        {
            int frames_in_buffer = frames_per_buffer;

            if (i+frames_in_buffer>frame_count)
                frames_in_buffer = frame_count-i;

            std::vector<uint8_t> block;
            auto out = std::back_inserter(block);

            write4( out, 0x464C494D );
            write2( out, frames_in_buffer );
            for (int j=0;j!=frames_in_buffer;j++)
            {
                const typename flimcompressor<W,H>::frame &f = frames[i+j];
                write2( out, f.ticks*370+8 );           //  size of sound + header + size itself
                write2( out, 0 );                       //  ffMode
                write4( out, 65536 );                   //  rate
                write( out, f.audio );
                write2( out, f.video.size()*4+2 );
                write( out, f.video );
            }

            auto out_movie = std::back_inserter( movie );
            write4( out_movie, block.size()+4 );
            write( out_movie, block );
        }

        FILE *movie_file = fopen( flim_pathname.c_str(), "wb" );
        fwrite( movie.data(), movie.size(), 1, movie_file );
        fclose( movie_file );
    */
    }
};

#endif
