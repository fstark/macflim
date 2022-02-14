#ifndef FLIMENCODER_INCLUDED__
#define FLIMENCODER_INCLUDED__

#include <string>

#include "flimcompressor.hpp"

#include "reader.hpp"
#include "writer.hpp"
#include <sstream>

extern bool sDebug;

/**
 * A set of encoding parameters
 */
class encoding_profile
{
protected:
    size_t W_ = 512;
    size_t H_ = 342;

    size_t byterate_ = 2000;
    double stability_ = 0.3;
    int fps_ratio_ = 1;
    bool group_ = true;
    std::string filters_ = "c";
    bool bars_ = true;              //  Do we put black bars around the image?

    image::dithering dither_ = image::error_diffusion;
    std::string error_algorithm_ = "floyd";
    float error_bleed_ = 1;
    bool error_bidi_ = false;

    bool silent_ = false;

    std::vector<flimcompressor::codec_spec> codecs_;

public:

    size_t width() const { return W_; }
    size_t height() const { return H_; }
    void set_size( size_t W, size_t H ) { W_ = W; H_ = H; }
    void set_width( size_t W ) { W_ = W; }
    void set_height( size_t H ) { H_ = H; }


    size_t byterate() const { return byterate_; }
    void set_byterate( size_t byterate ) { byterate_ = byterate; }

        //  Technically, we could put the half-rate/fps_ratio mecanism in the reader phase
        //  to avoid reading unecessary images, but it is more generic to put it here
        //  as it could allows to extend to dynamic half rate [yagni]
    int fps_ratio() const { return fps_ratio_; }
    void set_fps_ratio( int fps_ratio ) { fps_ratio_ = fps_ratio; }

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
        else if (dither=="error")
            dither_ = image::error_diffusion;
        else
            throw "Wrong dither option : only 'ordered' and 'error' are supported";
        return true;
    }
    void set_dither( image::dithering dither ) { dither_ = dither; }

    std::string error_algorithm() const { return error_algorithm_; }
    void set_error_algorithm( const std::string algo ) { error_algorithm_ = algo; }

    float error_bleed() const { return error_bleed_; }
    void set_error_bleed( float bleed ) { error_bleed_ = bleed; }

    bool error_bidi() const { return error_bidi_; }
    void set_error_bidi( bool error_bidi ) { error_bidi_ = error_bidi; }

    double stability() const { return stability_; }
    void set_stability( double stability ) { stability_ = stability; }

    const std::vector<flimcompressor::codec_spec> &codecs() const { return codecs_; }
    void set_codecs( const std::vector<flimcompressor::codec_spec> &codecs ) { codecs_ = codecs; }

    bool silent() const { return silent_; }
    void set_silent( bool silent ) { silent_ = silent; }

    static bool profile_named( const std::string name, size_t width, size_t height, encoding_profile &result )
    {
        result.set_size( width, height );
        if (name=="xl"s)
        {
            result.set_byterate( 580 );
            result.set_filters( "g1.6bbscz" );
            result.set_fps_ratio( 4 );
            result.set_group( true );
            result.set_stability( 0.5 );
            result.set_bars( true );
            result.set_dither( "ordered" );
            result.set_error_algorithm( "floyd" );
            result.set_error_bidi( true );
            result.set_error_bleed( 0.95 );
            result.codecs_.clear();
            result.codecs_.push_back( flimcompressor::make_codec( "null", result.W_, result.H_ ) );
            result.codecs_.push_back( flimcompressor::make_codec( "z32", result.W_, result.H_ ) );
            result.codecs_.push_back( flimcompressor::make_codec( "lines:count=10", result.W_, result.H_ ) );
            result.codecs_.push_back( flimcompressor::make_codec( "invert", result.W_, result.H_ ) );
            result.set_silent( true );
            return true;
        }
        if (name=="512"s)
        {
            result.set_byterate( 480 );
            result.set_filters( "g1.6bbscz" );
            result.set_fps_ratio( 4 );
            result.set_group( false );
            result.set_stability( 0.5 );
            result.set_bars( true );
            result.set_dither( "ordered" );
            result.set_error_algorithm( "floyd" );
            result.set_error_bidi( true );
            result.set_error_bleed( 0.95 );
            result.codecs_.clear();
            result.codecs_.push_back( flimcompressor::make_codec( "null", result.W_, result.H_ ) );
            result.codecs_.push_back( flimcompressor::make_codec( "z32", result.W_, result.H_ ) );
            result.codecs_.push_back( flimcompressor::make_codec( "lines:count=10", result.W_, result.H_ ) );
            result.codecs_.push_back( flimcompressor::make_codec( "invert", result.W_, result.H_ ) );
            result.set_silent( true );
            return true;
        }
        if (name=="plus"s)
        {
            result.set_byterate( 1500 );
            result.set_filters( "g1.6bbscz" );
            result.set_fps_ratio( 2 );
            result.set_group( false );
            result.set_stability( 0.5 );
            result.set_bars( true );
            result.set_dither( "ordered" );
            result.set_error_algorithm( "floyd" );
            result.set_error_bidi( true );
            result.set_error_bleed( 0.95 );
            result.codecs_.clear();
            result.codecs_.push_back( flimcompressor::make_codec( "null", result.W_, result.H_ ) );
            result.codecs_.push_back( flimcompressor::make_codec( "z32", result.W_, result.H_ ) );
            result.codecs_.push_back( flimcompressor::make_codec( "lines:count=30", result.W_, result.H_ ) );
            result.codecs_.push_back( flimcompressor::make_codec( "invert", result.W_, result.H_ ) );
            result.set_silent( false );
            return true;
        }
        if (name=="se"s)
        {
            result.set_byterate( 2500 );
            result.set_filters( "g1.6bsc" );
            result.set_fps_ratio( 2 );
            result.set_group( false );
            result.set_stability( 0.5 );
            result.set_bars( true );
            result.set_dither( "error" );
            result.set_error_algorithm( "floyd" );
            result.set_error_bidi( true );
            result.set_error_bleed( 0.98 );
            result.codecs_.clear();
            result.codecs_.push_back( flimcompressor::make_codec( "null", result.W_, result.H_ ) );
            result.codecs_.push_back( flimcompressor::make_codec( "z32", result.W_, result.H_ ) );
            result.codecs_.push_back( flimcompressor::make_codec( "lines:count=50", result.W_, result.H_ ) );
            result.codecs_.push_back( flimcompressor::make_codec( "invert", result.W_, result.H_ ) );
            result.set_silent( false );
            return true;
        }
        if (name=="se30"s)
        {
            result.set_byterate( 6000 );
            result.set_filters( "g1.6sc" );
            result.set_fps_ratio( 1 );
            result.set_group( true );
            result.set_stability( 0.3 );
            result.set_bars( false );
            result.set_dither( "error" );
            result.set_error_algorithm( "floyd" );
            result.set_error_bidi( true );
            result.set_error_bleed( 0.99 );
            result.codecs_.clear();
            result.codecs_.push_back( flimcompressor::make_codec( "null", result.W_, result.H_ ) );
            result.codecs_.push_back( flimcompressor::make_codec( "z32", result.W_, result.H_ ) );
            result.codecs_.push_back( flimcompressor::make_codec( "lines:count=70", result.W_, result.H_ ) );
            result.codecs_.push_back( flimcompressor::make_codec( "invert", result.W_, result.H_ ) );
            result.set_silent( false );
            return true;
        }
        if (name=="perfect"s)
        {
            result.set_byterate( 32000 );
            result.set_filters( "g1.6sc" );
            result.set_fps_ratio( 1 );
            result.set_group( true );
            result.set_stability( 0.3 );
            result.set_bars( false );
            result.set_dither( "error" );
            result.set_error_algorithm( "floyd" );
            result.set_error_bidi( true );
            result.set_error_bleed( 1 );
            result.codecs_.clear();
            result.codecs_.push_back( flimcompressor::make_codec( "null", result.W_, result.H_ ) );
            result.codecs_.push_back( flimcompressor::make_codec( "z32", result.W_, result.H_ ) );
            result.codecs_.push_back( flimcompressor::make_codec( "lines:count=342", result.W_, result.H_ ) );
            result.codecs_.push_back( flimcompressor::make_codec( "invert", result.W_, result.H_ ) );
            result.set_silent( false );
            return true;
        }

        return false;
    }

    std::string dither_string() const
    {
        switch (dither_)
        {
            case image::error_diffusion:
                return "error";
            case image::ordered:
                return "ordered";
        }
        return "???";
    }

    std::string description() const
    {
        char buffer[1024];

        std::ostringstream cmd;

        cmd << "--byterate " << byterate_;
        cmd << " --fps-ratio " << fps_ratio_;
        cmd << " --group " << (group_?"true":"false");
        cmd << " --bars " << (bars_?"true":"false");
        cmd << " --dither " << dither_string();
        if (dither_==image::error_diffusion)
        {
            cmd << " --error-stability " << stability_;
            cmd << " --error-algorithm " << error_algorithm_;
            cmd << " --error-bidi " << error_bidi_;
            cmd << " --error-bleed " << error_bleed_;
        }
        cmd << " --filters " << filters_;

        for (auto &c:codecs_)
            cmd << " --codec " << c.coder->description();

        cmd << " --silent " << (silent_?"true":"false");

        return cmd.str();
    }
};

#include "subtitles.hpp"

class flimencoder
{
    const encoding_profile &profile_;

    std::string out_pattern_ = "out-%06d.pgm"s;
    std::string change_pattern_ = "change-%06d.pgm"s;
    std::string diff_pattern_ = "diff-%06d.pgm"s;
    std::string target_pattern_ = "target-%06d.pgm"s;

    std::vector<subtitle> subtitles_;

    std::vector<image> images_;
    std::vector<sound_frame_t> audio_samples_;

    double fps_ = 24;
    double poster_ts_ = 0;

    std::string comment_;

    std::string watermark_;

    size_t cover_begin_;        /// Begin index of cover image
    size_t cover_end_;          /// End index of cover image 

    size_t frame_from_image( size_t n ) const
    {
        return ticks_from_frame( n-1, fps_/profile_.fps_ratio() );
    }

#if 0
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
#endif

    void fix()
    {
        //  TODO: make sure images and sound size matches

        std::clog << "**** fps               : " << fps_ << "/" << profile_.fps_ratio() << "=" << fps_/profile_.fps_ratio() << "\n";
        std::clog << "**** # of input images : " << images_.size() << "\n";
        std::clog << "**** # of movie ticks  : " << frame_from_image(images_.size()+1) << "\n";
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
    flimencoder( const encoding_profile &profile ) : profile_{ profile } {}

    void set_fps( double fps ) { fps_ = fps; }
    void set_comment( const std::string comment ) { comment_ = comment; }
    void set_cover( size_t cover_begin, size_t cover_end ) { cover_begin_ = cover_begin; cover_end_ = cover_end; }
    void set_watermark( const std::string watermark ) { watermark_ = watermark; }
    void set_out_pattern( const std::string pattern ) { out_pattern_ = pattern; }
    void set_diff_pattern( const std::string pattern ) { diff_pattern_ = pattern; }
    void set_change_pattern( const std::string pattern ) { change_pattern_ = pattern; }
    void set_target_pattern( const std::string pattern ) { target_pattern_ = pattern; }
    void set_poster_ts( double poster_ts ) { poster_ts_ = poster_ts; }
    void set_subtitles( const std::vector<subtitle> &subtitles ) { subtitles_ = subtitles; /* yes, it is a copy */ }

    //  Encode all the frames
    void make_flim( const std::string flim_pathname, input_reader *reader, const std::vector<std::unique_ptr<output_writer>> &writers )
    {  
        assert( reader );

        int i = 0;
        while (auto next = reader->next())
        {
            if ((i%profile_.fps_ratio())==0)
                images_.push_back( *next );
            i++;
        }

        assert( images_.size()>0 );

        //  Poster extraction
        image poster_image = images_[0];
        int poster_index = poster_ts_*fps_/profile_.fps_ratio();

        if (poster_index>=0 && poster_index<images_.size())
            poster_image = images_[poster_index];

std::cout << "POSTER INDEX: " << poster_index << "\n";

        auto filters_string = profile_.filters();
        poster_image = filter( poster_image, filters_string.c_str() );


        image poster_small( 128, 86 );
        copy( poster_small, poster_image, false );

        image previous( poster_small.W(), poster_small.H() );
        fill( previous, 0 );

        // auto prev = poster_small;
        auto poster_small_bw = poster_small;
        // auto error_diff = get_error_diffusion_by_name( "floyd" );


        if (profile_.dither()==image::error_diffusion)
            error_diffusion( poster_small_bw, poster_small, previous, 0, *get_error_diffusion_by_name( profile_.error_algorithm() ), profile_.error_bleed(), profile_.error_bidi() );
        else if (profile_.dither()==image::ordered)
            ordered_dither( poster_small_bw, poster_small, previous );

        // error_diffusion( poster_small_bw, poster_small, prev, 0, *error_diff, 0.99, true );
        write_image( "/tmp/poster1.pgm", poster_image );
        write_image( "/tmp/poster2.pgm", poster_small );
        write_image( "/tmp/poster3.pgm", poster_small_bw );

        if (!profile_.silent())
            while (auto next = reader->next_sound())
            {
                audio_samples_.push_back( *next );
            }

        // audio_samples_ = normalize_sound( reader->raw_sound(), images_.size()/fps_*60*370 );

        fix();

        flimcompressor fc{ profile_.width(), profile_.height(), images_, audio_samples_, fps_ / profile_.fps_ratio(), subtitles_ };

        fc.compress( profile_.stability(), profile_.byterate(), profile_.group(), profile_.filters(), watermark_, profile_.codecs(), profile_.dither(), profile_.bars(), profile_.error_algorithm(), profile_.error_bleed(), profile_.error_bidi() );

        if (out_pattern_!="") delete_files_of_pattern( out_pattern_ );
        if (diff_pattern_!="") delete_files_of_pattern( diff_pattern_ );
        if (change_pattern_!="") delete_files_of_pattern( change_pattern_ );
        if (target_pattern_!="") delete_files_of_pattern( target_pattern_ );

        auto frames = fc.get_frames();

        std::vector<uint8_t> movie; //  #### Should be 'frames'
        auto out_movie = std::back_inserter( movie );

        framebuffer previous_frame{ profile_.width(), profile_.height() };
        previous_frame.fill( 0xff );

        if (sDebug)
            std::clog << "GENERATING ENCODED MOVIE AND PGM FILES\n";

        std::vector<uint8_t> toc;
        auto out_toc = std::back_inserter( toc );

        if (out_pattern_!="")   //  generate posters samples
        {
            for (auto &poster_source:images_)
            {
                static int img = 1;
                char buffer[1024];

                sprintf( buffer, out_pattern_.c_str(), img++ );
                image poster_small( 128, 86 );
                copy( poster_small, poster_source, false );

                auto prev = poster_small;
                auto poster_small_bw = poster_small;
                auto error_diff = get_error_diffusion_by_name( "floyd" );

                error_diffusion( poster_small_bw, poster_small, prev, 0, *error_diff, 0.99, true );
                write_image( buffer, poster_small_bw );
            }
        }

        auto current_frame = std::begin(frames);
        while (current_frame!=std::end(frames))
        {
            //  logs current image
            {
                static int img = 1;
                char buffer[1024];
                if (out_pattern_!="")
                {
                    sprintf( buffer, out_pattern_.c_str(), img );
                    // auto logimg = current_frame->result.as_image();
                    // write_image( buffer, logimg );
        // image poster_small( 128, 86 );
        // copy( poster_small, images_[img-1], false );

        // auto prev = poster_small;
        // auto poster_small_bw = poster_small;
        // auto error_diff = get_error_diffusion_by_name( "floyd" );

        // error_diffusion( poster_small_bw, poster_small, prev, 0, *error_diff, 0.99, true );
        // write_image( buffer, poster_small_bw );

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

            size_t frame_start = movie.size();

            write2( out_movie, current_frame->ticks );

            if (!profile_.silent())
            {
                write2( out_movie, current_frame->ticks*370+8 );           //  size of sound + header + size itself
                write2( out_movie, 0 );                       //  ffMode
                write4( out_movie, 65536 );                   //  rate
                write( out_movie, current_frame->audio );
            }
            else
            {
                write2( out_movie, 2 );           //  No sound
            }
            write2( out_movie, current_frame->video.size()+2 );
            write( out_movie, current_frame->video );

            //  TOC entry for current frame
            write2( out_toc, movie.size()-frame_start );

            current_frame++;
        }

        std::vector<u_int8_t> global;
        auto out_global = std::back_inserter( global );

        write2( out_global, profile_.width() );                      //  width
        write2( out_global, profile_.height() );                      //  height
        write2( out_global, profile_.silent() );        //  1 = silent
        write4( out_global, frames.size() );            //  Framecount
        size_t total_ticks = std::accumulate( std::begin(frames), std::end(frames), 0, []( size_t a, flimcompressor::frame &f ){ return a+f.ticks; } );
        write4( out_global, total_ticks );              //  Tick count

std::cout << "PROFILE BYTERATE " << profile_.byterate() << "\n";
        write2( out_global, profile_.byterate() );      //  Byterate

        framebuffer poster_fb{ poster_small_bw };
        std::vector<u_int8_t> poster = poster_fb.raw_values_natural<u_int8_t>();

        std::vector<uint8_t> header;
        auto out_header = std::back_inserter( header );

        const int HEADER_SIZE = 64;

        write2( out_header, 0x1 );                      //  Version
        write2( out_header, 4 );                        //  Entry count

        write2( out_header, 0x00 ); //  Info
        write4( out_header, 0 );  //  TOC offset
        write4( out_header, global.size() );            //  Frame count

        write2( out_header, 0x01 ); //  MOVIE
        write4( out_header, global.size() );
        write4( out_header, movie.size() );          

        write2( out_header, 0x02 ); //  TOC
        write4( out_header, global.size()+movie.size() ); 
        write4( out_header, toc.size() );            

        write2( out_header, 0x03 ); //  POSTER
        write4( out_header, global.size()+movie.size()+toc.size() );
        write4( out_header, poster.size() );             

        if (sDebug)
            std::clog << "WRITING FLIM FILE\n";

        FILE *movie_file = fopen( flim_pathname.c_str(), "wb" );

        char buffer[1024];
        std::fill( std::begin(buffer), std::end(buffer), 0 );
        strcpy( buffer, comment_.c_str() );
        fwrite( buffer, 1022, 1, movie_file );

            //  Computes checksum
        long fletcher = 0;
        if ((movie.size()%2)==1)
            movie.push_back( 0x00 );
        for (int i=0;i!=header.size();i+=2)
        {
            fletcher += ((int)(header[i]))*256+header[i+1];
            fletcher %= 65535;
        }
        for (int i=0;i!=global.size();i+=2)
        {
            fletcher += ((int)(global[i]))*256+global[i+1];
            fletcher %= 65535;
        }
        for (int i=0;i!=movie.size();i+=2)
        {
            fletcher += ((int)(movie[i]))*256+movie[i+1];
            fletcher %= 65535;
        }
        for (int i=0;i!=toc.size();i+=2)
        {
            fletcher += ((int)(toc[i]))*256+toc[i+1];
            fletcher %= 65535;
        }
        for (int i=0;i!=poster.size();i+=2)
        {
            fletcher += ((int)(poster[i]))*256+poster[i+1];
            fletcher %= 65535;
        }
        uint8_t b = fletcher/256;
        fwrite( &b, 1, 1, movie_file );
        b = fletcher%256;
        fwrite( &b, 1, 1, movie_file );

        fwrite( header.data(), header.size(), 1, movie_file );
        fwrite( global.data(), global.size(), 1, movie_file );
        fwrite( movie.data(), movie.size(), 1, movie_file );
        fwrite( toc.data(), toc.size(), 1, movie_file );
        fwrite( poster.data(), poster.size(), 1, movie_file );

        fclose( movie_file );

        if (writers.size())
        {

            size_t index = 0;

            for (auto &writer:writers)
            {
                auto sound = std::begin(audio_samples_);

                //  Generate the mp4 file
                for (auto &frame:frames)
                {
                    // std::clog << frame.ticks << std::flush;
                    for (int i=0;i!=frame.ticks;i++)
                    {
                        index++;
                        std::clog << "Wrote " << index << " frames\r" << std::flush;
                        sound_frame_t snd;

                        if (!profile_.silent())
                            if (sound<std::end(audio_samples_))
                                snd = *sound++;

                        writer->write_frame( frame.result.as_image(), snd );
                    }
                }
                std::clog << "\n";
            }
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
