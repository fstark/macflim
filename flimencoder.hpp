#ifndef FLIMENCODER_INCLUDED__
#define FLIMENCODER_INCLUDED__

#include <string>

#include "flimcompressor.hpp"

#include "reader.hpp"
#include "writer.hpp"


/**
 * A set of encoding parameters
 */
class encoding_profile
{
protected:
    size_t W_ = 512;
    size_t H_ = 342;

    size_t byterate_ = 2000;
    size_t buffer_size_ = 300000;
    double stability_ = 0.3;
    bool half_rate_ = false;
    bool group_ = true;
    std::string filters_ = "c";
    bool bars_ = true;              //  Do we put black bars around the image?

    image::dithering dither_ = image::floyd_steinberg;

    std::vector<flimcompressor::codec_spec> codecs_;

public:

    size_t width() const { return W_; }
    size_t height() const { return H_; }
    void set_size( size_t W, size_t H ) { W_ = W; H_ = H; }

    size_t byterate() const { return byterate_; }
    void set_byterate( size_t byterate ) { byterate_ = byterate; }

    size_t buffer_size() const { return buffer_size_; }
    void set_buffer_size( size_t buffer_size ) { buffer_size_ = buffer_size; }

        //  Technically, we could put the half-rate mecanism in the reader phase
        //  to avoid reading unecessary images, but it is more generic to put it here
        //  as it allows to extend to dynamic half rate
    bool half_rate() const { return half_rate_; }
    void set_half_rate( bool half_rate ) { half_rate_ = half_rate; }

    bool group() const { return group_; }
    void set_group( bool group ) { group_ = group; }

    std::string filters() const { return filters_; }
    void set_filters( const std::string filters ) { filters_ = filters; }

    bool bars() const { return bars_; }
    void set_bars( bool bars ) { bars_ = bars; }

    image::dithering dither() const { return dither_; }
    bool set_dither( std::string dither )
    {
        if (dither=="ordered")
            dither_ = image::ordered;
        else if (dither=="floyd")
            dither_ = image::floyd_steinberg;
        else
            return false;
        return true;
    }
    void set_dither( image::dithering dither ) { dither_ = dither; }

    double stability() const { return stability_; }
    void set_stability( double stability ) { stability_ = stability; }

    const std::vector<flimcompressor::codec_spec> &codecs() const { return codecs_; }
    void set_codecs( const std::vector<flimcompressor::codec_spec> &codecs ) { codecs_ = codecs; }

    static bool profile_named( const std::string name, encoding_profile &result )
    {
        result.set_size( 512, 342 );
        result.set_buffer_size( 300000 );

        if (name=="plus"s)
        {
            result.set_byterate( 1500 );
            result.set_filters( "gbbscz" );
            result.set_half_rate( true );
            result.set_group( false );
            result.set_dither( "ordered" );
            result.set_stability( 0.5 );
            result.codecs_.clear();
            result.codecs_.push_back( flimcompressor::make_codec( "null", result.W_, result.H_ ) );
            result.codecs_.push_back( flimcompressor::make_codec( "z32", result.W_, result.H_ ) );
            result.codecs_.push_back( flimcompressor::make_codec( "lines:count=30", result.W_, result.H_ ) );
            result.codecs_.push_back( flimcompressor::make_codec( "invert", result.W_, result.H_ ) );
            return true;
        }
        if (name=="se"s) 
        {
            result.set_byterate( 2500 );
            result.set_filters( "gbsc" );
            result.set_half_rate( true );
            result.set_group( false );
            result.set_dither( "floyd" );
            result.set_stability( 0.5 );
            result.codecs_.clear();
            result.codecs_.push_back( flimcompressor::make_codec( "null", result.W_, result.H_ ) );
            result.codecs_.push_back( flimcompressor::make_codec( "z32", result.W_, result.H_ ) );
            result.codecs_.push_back( flimcompressor::make_codec( "lines:count=50", result.W_, result.H_ ) );
            result.codecs_.push_back( flimcompressor::make_codec( "invert", result.W_, result.H_ ) );
            return true;
        }
        if (name=="se30"s)
        {
            result.set_byterate( 6000 );
            result.set_filters( "gsc" );
            result.set_half_rate( false );
            result.set_group( true );
            result.set_dither( "floyd" );
            result.set_stability( 0.3 );
            result.codecs_.clear();
            result.codecs_.push_back( flimcompressor::make_codec( "null", result.W_, result.H_ ) );
            result.codecs_.push_back( flimcompressor::make_codec( "z32", result.W_, result.H_ ) );
            result.codecs_.push_back( flimcompressor::make_codec( "lines:count=70", result.W_, result.H_ ) );
            result.codecs_.push_back( flimcompressor::make_codec( "invert", result.W_, result.H_ ) );
            return true;
        }
        if (name=="perfect"s)
        {
            result.set_byterate( 32000 );
            result.set_filters( "gsc" );
            result.set_half_rate( false );
            result.set_group( true );
            result.set_dither( "floyd" );
            result.set_stability( 0.3 );
            result.codecs_.clear();
            result.codecs_.push_back( flimcompressor::make_codec( "null", result.W_, result.H_ ) );
            result.codecs_.push_back( flimcompressor::make_codec( "z32", result.W_, result.H_ ) );
            result.codecs_.push_back( flimcompressor::make_codec( "lines:count=342", result.W_, result.H_ ) );
            result.codecs_.push_back( flimcompressor::make_codec( "invert", result.W_, result.H_ ) );
            return true;
        }

        return false;
    }

    std::string dither_string() const
    {
        switch (dither_)
        {
            case image::floyd_steinberg:
                return "floyd";
            case image::ordered:
                return "ordered";
        }
        return "???";
    }

    std::string description() const
    {
        char buffer[1024];
        sprintf( buffer, "br:%ld st:%.2f%s%s%s fi:%s %s\n", byterate_, stability_, half_rate_?" half-rate":" full-rate", group_?" group":" no-group", bars_?" bars":" no-bars", filters_.c_str(), dither_string().c_str() );
        std::string res = buffer;
        for (auto &c:codecs_)
        {
            char buffer2[1024];
            sprintf( buffer2, "%d %.2g*%s\n", c.signature, c.penality, c.coder->description().c_str() );
            res += buffer2;
        }
        return res;
    }
};

class flimencoder
{
    const encoding_profile &profile_;

    const std::string in_;
    const std::string audio_;

    std::string out_pattern_ = "out-%06d.pgm"s;
    std::string change_pattern_ = "change-%06d.pgm"s;
    std::string diff_pattern_ = "diff-%06d.pgm"s;
    std::string target_pattern_ = "target-%06d.pgm"s;

    std::vector<image> images_;
    std::vector<sound_frame_t> audio_samples_;

    double fps_ = 24;

    std::string comment_;

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

            image img( profile_.width(), profile_.height() );

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

    void fix()
    {
        //  TODO: make sure images and sound size matches

        std::clog << "**** fps                : " << fps_ << "\n";

        std::clog << "**** # of video images  : " << images_.size() << "\n";
        std::clog << "**** # of video frames  : " << frame_from_image(images_.size()+1) << "\n";
    }

    int clamp( double v, int a, int b )
    {
        int res = v+0.5;
        if (res<a) res = a;
        if (res>b) res = b;
        return res;
    }

    std::vector<u_int8_t> normalize_sound( std::vector<double> sound_samples, size_t len )
    {
        sound_samples.resize(len);
        std::vector<u_int8_t> res;

        if (sound_samples.size()>0)
        {
            auto mi = std::min_element( std::begin(sound_samples), std::end(sound_samples) );
            auto ma = std::max_element( std::begin(sound_samples), std::end(sound_samples) );
            double scale = std::max( ::fabs(*mi), ::fabs(*ma) );
            std::transform( std::begin(sound_samples), std::end(sound_samples), std::back_inserter(res), [&]( double v ) { return clamp( (v/scale)*128+128, 0, 255 ); } );
            std::clog   << "Normalized  sound : [" << *mi << "," << *ma << "] => ["
                        << (int)*std::min_element( std::begin(res), std::end(res) ) << ","
                        << (int)*std::max_element( std::begin(res), std::end(res) ) << "]\n"; 
        }
        else
        {
            std::clog << "SOUND IS EMPTY\n";
        }

        return res;
    }

public:
//  #### remove in and audio
    flimencoder( const encoding_profile &profile, const std::string &in, const std::string &audio ) : profile_{ profile }, in_{in}, audio_{audio} {}

    void set_fps( double fps ) { fps_ = fps; }
    void set_comment( const std::string comment ) { comment_ = comment; }
    void set_cover( size_t cover_begin, size_t cover_end ) { cover_begin_ = cover_begin; cover_end_ = cover_end; }
    void set_watermark( const std::string watermark ) { watermark_ = watermark; }
    void set_out_pattern( const std::string pattern ) { out_pattern_ = pattern; }
    void set_diff_pattern( const std::string pattern ) { diff_pattern_ = pattern; }
    void set_change_pattern( const std::string pattern ) { change_pattern_ = pattern; }
    void set_target_pattern( const std::string pattern ) { target_pattern_ = pattern; }

    //  Encode all the blocks
    void make_flim( const std::string flim_pathname, input_reader *reader, const std::vector<std::unique_ptr<output_writer>> &writers )
    {  
        assert( reader );
/*
        read_images( from, to, profile_.half_rate() );
        read_audio( from, images_.size() );
*/

        while (auto next = reader->next())
        {
            images_.push_back( *next );
        }

        while (auto next = reader->next_sound())
        {
            audio_samples_.push_back( *next );
        }

        // audio_samples_ = normalize_sound( reader->raw_sound(), images_.size()/fps_*60*370 );

        fix();

        flimcompressor fc{ profile_.width(), profile_.height(), images_, audio_samples_, fps_ };

        fc.compress( profile_.stability(), profile_.byterate(), profile_.group(), profile_.filters(), watermark_, profile_.codecs(), profile_.dither(), profile_.bars() );

        if (out_pattern_!="") delete_files_of_pattern( out_pattern_ );
        if (diff_pattern_!="") delete_files_of_pattern( diff_pattern_ );
        if (change_pattern_!="") delete_files_of_pattern( change_pattern_ );
        if (target_pattern_!="") delete_files_of_pattern( target_pattern_ );

        auto frames = fc.get_frames();

        std::vector<uint8_t> movie;
        auto out_movie = std::back_inserter( movie );

        auto block_first_frame = std::begin(frames);

        framebuffer previous_frame{ profile_.width(), profile_.height() };
        previous_frame.fill( 0xff );

        std::clog << "GENERATING ENCODED MOVIE AND PGM FILES\n";

        auto current_frame = block_first_frame;
        while(current_frame!=std::end(frames))
        {
            std::vector<u_int8_t> block_content;
            auto block_ptr = std::back_inserter( block_content );
            size_t current_block_size = 0;

            while (current_frame!=std::end(frames) && current_block_size+current_frame->get_size()<profile_.buffer_size())
            {
                //  logs current image
                {
                    static int img = 1;
                    char buffer[1024];
                    if (out_pattern_!="")
                    {
                        sprintf( buffer, out_pattern_.c_str(), img );
                        auto logimg = current_frame->result.as_image();
                        write_image( buffer, logimg );
                    }
                    if (diff_pattern_!="")
                    {
                        sprintf( buffer, diff_pattern_.c_str(), img );
                        auto logimg = (current_frame->result^current_frame->source).inverted().as_image();
                        write_image( buffer, logimg );
                    }
                    if (change_pattern_!="")
                    {
                        sprintf( buffer, change_pattern_.c_str(), img );
                        auto logimg = (current_frame->result^previous_frame).inverted().as_image();
                        write_image( buffer, logimg );
                        previous_frame = current_frame->result;
                    }
                    if (target_pattern_!="")
                    {
                        sprintf( buffer, target_pattern_.c_str(), img );
                        auto logimg = current_frame->source.as_image();
                        write_image( buffer, logimg );
                    }
                    img++;
                }

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

        std::clog << "WRITING FLIM FILE\n";

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

        for (auto &writer:writers)
        {
            std::clog << "GENERATING ADDITIONAL OUTPUT...\n";
            auto sound = std::begin(audio_samples_);

            //  Generate the mp4 file
            for (auto &frame:frames)
            {
                std::clog << frame.ticks << std::flush;
                for (int i=0;i!=frame.ticks;i++)
                {
                    sound_frame_t snd;
                    if (sound<std::end(audio_samples_))
                        snd = *sound++;

                    writer->write_frame( frame.result.as_image(), snd );
                }
            }
            std::clog << "...DONE\n";
        }

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
    }
};

#endif
