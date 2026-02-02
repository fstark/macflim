#ifndef FLIMENCODER_INCLUDED__
#define FLIMENCODER_INCLUDED__

#include <string>

#include "flimcompressor.hpp"
#include "flimformat.hpp"

#include "reader.hpp"
#include "writer.hpp"
//#include <sstream>

extern bool sDebug;


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
        size_t poster_index = poster_ts_*fps_/profile_.fps_ratio();

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


//  Flim file generation
        auto frames = fc.get_frames();
        encoded_flim ef{ comment_ };
        flim_info fi{ profile_, frames };
        ef.add( fi );
        ef.add( frames );
        ef.add_poster( poster_small_bw );

        if (fc.get_initial())
            ef.add_initial( *fc.get_initial() );

        FILE *movie_file = fopen( flim_pathname.c_str(), "wb" );
        ef.fwrite( movie_file );
        fclose( movie_file );

//  Generate mp4 and gif
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
                    for (size_t i=0;i!=frame.ticks;i++)
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

        if (out_pattern_!="") delete_files_of_pattern( out_pattern_ );
        if (diff_pattern_!="") delete_files_of_pattern( diff_pattern_ );
        if (change_pattern_!="") delete_files_of_pattern( change_pattern_ );
        if (target_pattern_!="") delete_files_of_pattern( target_pattern_ );

        // auto frames = fc.get_frames();

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

        write2( out_header, 0x1 );                      //  Version
        write2( out_header, 4 );                        //  Entry count

        write2( out_header, 0x00 );                     //  Info
        write4( out_header, 0 );
        write4( out_header, global.size() );

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


        char buffer[1024];
        std::fill( std::begin(buffer), std::end(buffer), 0 );
        strcpy( buffer, comment_.c_str() );
        fwrite( buffer, 1022, 1, movie_file );

            //  Computes checksum
        long fletcher = 0;
        if ((movie.size()%2)==1)
            movie.push_back( 0x00 );
        for (size_t i=0;i!=header.size();i+=2)
        {
            fletcher += ((int)(header[i]))*256+header[i+1];
            fletcher %= 65535;
        }

printf( "FLETCHER HEADER == %ld\n", fletcher );
        for (size_t i=0;i!=global.size();i+=2)
        {
            fletcher += ((int)(global[i]))*256+global[i+1];
            fletcher %= 65535;
        }
printf( "FLETCHER 0 == %ld\n", fletcher );
        for (size_t i=0;i!=movie.size();i+=2)
        {
            fletcher += ((int)(movie[i]))*256+movie[i+1];
            fletcher %= 65535;
        }
printf( "FLETCHER 1 == %ld\n", fletcher );
        for (size_t i=0;i!=toc.size();i+=2)
        {
            fletcher += ((int)(toc[i]))*256+toc[i+1];
            fletcher %= 65535;
        }
printf( "FLETCHER 2 == %ld\n", fletcher );
        for (size_t i=0;i!=poster.size();i+=2)
        {
            fletcher += ((int)(poster[i]))*256+poster[i+1];
            fletcher %= 65535;
        }
printf( "FLETCHER 3 == %ld\n", fletcher );
        uint8_t b = fletcher/256;
        fwrite( &b, 1, 1, movie_file );
        b = fletcher%256;
        fwrite( &b, 1, 1, movie_file );

return;

#if 0
        FILE *movie_file = fopen( flim_pathname.c_str(), "wb" );


        fwrite( header.data(), header.size(), 1, movie_file );
        fwrite( global.data(), global.size(), 1, movie_file );
        fwrite( movie.data(), movie.size(), 1, movie_file );
        fwrite( toc.data(), toc.size(), 1, movie_file );
        fwrite( poster.data(), poster.size(), 1, movie_file );

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
#endif
    }
};

#endif
