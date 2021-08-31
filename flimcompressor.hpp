#ifndef FILM_COMPRESSOR_INCLUDED__
#define FILM_COMPRESSOR_INCLUDED__

#include "image.hpp"
#include "framebuffer.hpp"
#include "imgcompress.hpp"
#include "compressor.hpp"

#include <vector>
#include <array>
#include <bitset>
#include <algorithm>
#include <memory>

#include "reader.hpp"

#define VERBOSE

using namespace std::string_literals;

/**
 * The flimcompressor manages higher aspects of the compression
 */

class flimcompressor
{
public:
    struct frame
    {
        frame( size_t W, size_t H ) : source{ W, H }, result{ W, H } {}

        framebuffer source;

        size_t ticks;
        std::vector<uint8_t> audio;
        std::vector<uint8_t> video; //  Encoded flim

        framebuffer result;

        size_t get_size() { return audio.size()+video.size()*4; }
    };

private:
    const std::vector<image> &images_;
    const std::vector<sound_frame_t> &audio_;
    const double fps_;

    std::vector<frame> frames_;

    size_t W_;
    size_t H_;

public:
    flimcompressor( size_t W, size_t H, const std::vector<image> &images, const std::vector<sound_frame_t> &audio, double fps ) : W_{W}, H_{H}, images_{images}, audio_{audio}, fps_{fps} {}

    const std::vector<frame> &get_frames() const { return frames_; }

    struct codec_spec
    {
        uint8_t signature;
        double penality;
        std::shared_ptr<compressor> coder;
    };

    static std::vector<std::string> split( const std::string s, const std::string delimiter )
    {
        size_t pos_start = 0, pos_end, delim_len = delimiter.length();
        std::string token;
        std::vector<std::string> res;

        while ((pos_end = s.find (delimiter, pos_start)) != std::string::npos)
        {
            token = s.substr (pos_start, pos_end - pos_start);
            pos_start = pos_end + delim_len;
            res.push_back (token);
        }

        res.push_back (s.substr (pos_start));
        return res;
    }

    static codec_spec make_codec( const std::string &spec_string, size_t W, size_t H )
    {
        auto spec_array = split( spec_string, ":" );
        auto name = spec_array[0];
        std::string parameters_string = "";

        if (spec_array.size()>1)
            parameters_string = spec_array[1];

        codec_spec spec;
        spec.signature = 0x00;
        spec.penality = 1;
        spec.coder = std::make_shared<null_compressor>();

        if (name=="z16")
        {
            spec.signature = 0x01;
            spec.penality = 0.45;
            spec.coder = std::make_shared<vertical_compressor<uint16_t>>( W, H, uint16_ruler::ruler );
        }
        else if (name=="z32")
        {   
            spec.signature = 0x02;
            spec.penality = 1.00;
            spec.coder = std::make_shared<vertical_compressor<uint32_t>>( W, H, uint32_ruler::ruler );
        }
        else if (name=="z32old")
        {   
            static bit_ruler<uint32_t> br32;
            spec.signature = 0x02;
            spec.penality = 1.00;
            spec.coder = std::make_shared<vertical_compressor<uint32_t>>( W, H, br32 );
        }
        else if (name=="invert")
        {
            spec.signature = 0x03;
            spec.penality = 1.00;
            spec.coder = std::make_shared<invert_compressor>();
        }
        else if (name=="lines")
        {   
            spec.signature = 0x04;
            spec.penality = 1.00;
            spec.coder = std::make_shared<copy_line_compressor>();
        }
        else if (name=="null")
        {   
            spec.signature = 0x00;
            spec.penality = 1.00;
            spec.coder = std::make_shared<null_compressor>();
        }
        else
        {
            std::clog << "Unknown codec : [" << name << "]\n";
            throw "Unknown codec";
        }

        for (auto &param_string:split(parameters_string,","))
        {
            auto v = split( param_string, "=" );
            std::string pname = v[0];;
            std::string pvalue = "";
            if (v.size()>1)
                pvalue = v[1];
            spec.coder->set_parameter( pname, pvalue );
        }

        return spec;
    }

    void compress( double stability, size_t byterate, bool group, const std::string &filters, const std::string &watermark, const std::vector<codec_spec> &codecs, image::dithering dither, bool bars, const std::string error_algorithm, float error_bleed, bool error_bidi )
    {
        image previous( W_, H_ );
        fill( previous, 0 );

        framebuffer current_fb{ previous };     //  The framebuffer displayed on screen at each step

        int in_fr=0;

        size_t current_tick = 0;

        const size_t BucketsCount = 1000;
        std::vector<size_t> fail;
        fail.resize( BucketsCount+1, 0 );

        double total_q = 0;

            //  The audio ptr
        auto audio = std::begin( audio_ );

        for (auto &big_image:images_)
        {
            image dest( W_, H_ );

            image source_image( 512, 342 );
            copy( source_image, big_image, bars );

            image img = filter( source_image, filters.c_str() );

            if (dither==image::error_diffusion)
                error_diffusion( dest, img, previous, stability, *get_error_diffusion_by_name( error_algorithm ), error_bleed, error_bidi );
            else if (dither==image::ordered)
                ordered_dither( dest, img, previous );
            else
                throw "Unknown dithering option";

            previous = dest;
            //  dest = filter( dest, "gsc" );
            
            round_corners( dest );
            ::watermark( dest, watermark );
            //  DEBUG frame count
            // {
            //     char buffer[1024];
            //     sprintf( buffer, "%zu", current_tick );
            //     ::watermark( dest, buffer );
            // }
            framebuffer fb{ dest };

                //  Let's see how many ticks we have to display this image
            in_fr++;
            size_t next_tick = ticks_from_frame( in_fr, fps_ );
            size_t ticks = next_tick-current_tick;
// std::clog << "current_tick:" << current_tick << " in_fr:" << in_fr << " next_tick:" << next_tick << " fps:" << fps_ << "\n";
            assert( ticks>0 );

            size_t local_ticks = 1;

            if (group)
                local_ticks = ticks;

            for (int i=0;i!=ticks;i+=local_ticks)
            {

                    //  Build the frame
                frame f{ W_, H_ };

                f.ticks = local_ticks;

                for (int i=0;i!=local_ticks;i++)
                {

                    sound_frame_t snd;
                    if (audio<std::end(audio_))
                        snd = *audio++;
                    std::copy( snd.begin(), snd.end(), std::back_inserter(f.audio) );
                }

                    //  What is the video budget?
                size_t video_budget = byterate*local_ticks;
               
                    //  Encode within that budget with every codec
                

                std::vector<std::vector<uint8_t>> encoded_datas;
                std::vector<double> qualities;
                std::vector<framebuffer> encoded_framebuffers;

                for (auto &codec:codecs)
                {
                    auto codec_current_fb = current_fb;
                    encoded_datas.push_back( codec.coder->compress( codec_current_fb, fb, /* weigths, */ video_budget*codec.penality ) );
                    qualities.push_back( codec_current_fb.proximity( fb ) );
                    encoded_framebuffers.push_back( codec_current_fb );     //  std::move ...
                }


#if 0
 Later:
                std::transform(
//                    std::execution::par_unseq,
                    std::begin(codecs),
                    std::end(codecs),
                    std::back_inserter(encoded_datas),
                    [&]( auto &codec ) { auto codec_current_fb = current_fb; return codec.coder->compress( codec_current_fb, fb, /* weigths, */ video_budget ); }
                );
#endif

                auto best_ix = std::max_element( std::begin(qualities), std::end(qualities) )-std::begin(qualities);

                    //  Extra verbosity for specific compile-time specified condition
                if (frames_.size()==178 && false)
                {
                    int img = frames_.size()+1;
                    std::clog << "CODEC log enabled for frame #" << img << ":\n";
                    for (int i=0;i!=codecs.size();i++)
                    {
                        char buffer[1024];
                        sprintf( buffer, "codec-%06d-img-%d.pgm", img, (int)codecs[i].signature );
                        auto logimg = encoded_framebuffers[i].as_image();
                        write_image( buffer, logimg );

                        sprintf( buffer, "codec-%06d-xor-%d.pgm", img, (int)codecs[i].signature );
                        auto logimg2 = (encoded_framebuffers[i]^current_fb).inverted().as_image();
                        write_image( buffer, logimg2 );

                        sprintf( buffer, "codec-%06d-yor-%d.pgm", img, (int)codecs[i].signature );
                        auto logimg3 = (encoded_framebuffers[i]^fb).inverted().as_image();
                        write_image( buffer, logimg3 );

                        std::clog << "  "
                                  << codecs[i].coder->name()
                                  << " bytes:" 
                                  << encoded_datas[i].size() 
                                  << " quality:"
                                  << qualities[i] 
                                  << " pixel_count:" 
                                  << (encoded_framebuffers[i]^current_fb).pixel_count() 
                                  << "\n";
                    }


                    char buffer[1024];
                    sprintf( buffer, "codec-%06d-img-target.pgm", img );
                    auto logimg = fb.as_image();
                    write_image( buffer, logimg );
                    fprintf( stderr, "\n" );
                }

                // std::clog << best_ix;        //  encoder ised

                f.video = encoded_datas[best_ix];
                f.video.insert( std::begin(f.video), codecs[best_ix].signature );
                f.video.insert( std::begin(f.video), 0x00 );
                f.video.insert( std::begin(f.video), 0x00 );
                f.video.insert( std::begin(f.video), 0x00 );

                current_fb = encoded_framebuffers[best_ix];

                f.source = fb;
                f.result = current_fb;

                frames_.push_back( f );
                std::clog << "Encoded " << frames_.size() << " output frames\r" << std::flush;
            }

            auto q = frames_.back().result.proximity( fb );

            total_q += q;
/*
            if (q!=1)
            {
                fprintf( stderr, "# %4d (%5.3fs) \u001b[%sm%05.3f%%\u001b[0m\n", in_fr, current_tick/60.0, q<.9?"91":"0", q*100 );
            }
*/
            fail[q*BucketsCount]++;

            current_tick = next_tick;
        }

        std::clog << "\n";

        bool dump_stats = false;    //  #### Move to argument

        if (dump_stats)
        {
            std::clog << "+----------+--------+----------+----------+\n";
            std::clog << "|     Q    | Frames |   Perc.  |  Cumul.  |\n";
            std::clog << "|----------|--------|----------|----------|\n";
        }
        size_t cumulative = 0;
        double var99 = 0;
        double var98 = 0;
        double var95 = 0;
        for (size_t i=0;i!=BucketsCount+1;i++)
        {
            cumulative += fail[i];
            auto percent = (cumulative*1.0/in_fr);
            if (percent>0.01 && var99==0)
                var99 = i*1.0/BucketsCount;
            if (percent>0.02 && var98==0)
                var98 = i*1.0/BucketsCount;
            if (percent>0.05 && var95==0)
                var95 = i*1.0/BucketsCount;
            if (dump_stats)
                if (fail[i])
                    fprintf( stderr, "| %7.3f%% | %6zu | %7.3f%% | %7.3f%% |\n", i*1.0/BucketsCount*100, fail[i], (fail[i]*1.0/in_fr)*100, percent*100 );
        }
        if (dump_stats)
            std::clog << "+----------+--------+----------+----------+\n";
        std::clog << var99*100 << "% of frames are within 1% of the target pixels\n";
        std::clog << var98*100 << "% of frames are within 2% of the target pixels\n";
        std::clog << var95*100 << "% of frames are within 5% of the target pixels\n";

        // fprintf( stderr, "\n\nFrames rendered at less than 00-90%%: %ld. Rendered at 90-99%%: %ld. Average rendering %7.5f%%.\n", fail2, fail1, total_q/in_fr*100 );
    }
};


#endif
