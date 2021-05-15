#ifndef FLIMENCODER_INCLUDED__
#define FLIMENCODER_INCLUDED__

#include <string>

#include "flimcompressor.hpp"

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

    std::string watermark_;

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

        auto min_sample = *std::min_element( std::begin(audio_samples_), std::end(audio_samples_) );
        auto max_sample = *std::max_element( std::begin(audio_samples_), std::end(audio_samples_) );
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
    void set_watermark( const std::string &watermark ) { watermark_ = watermark; }

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

        fc.compress( stability_, byterate_, group_, filters_, watermark_ );

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

            while (current_frame!=std::end(frames) && current_block_size+current_frame->get_size()<buffer_size_)
            {
                write2( block_ptr, current_frame->ticks*370+8 );           //  size of sound + header + size itself
                write2( block_ptr, 0 );                       //  ffMode
                write4( block_ptr, 65536 );                   //  rate
                write( block_ptr, current_frame->audio );
                write2( block_ptr, current_frame->video.size()+2 );
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
