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

#define VERBOSE

using namespace std::string_literals;


inline size_t ticks_from_frame( size_t n, double fps ) { return n/fps*60; }

/**
 * The flimcompressor manages higher aspects of the compression
 */

class flimcompressor
{
public:
    struct frame
    {
        frame( size_t W, size_t H ) : result{ W, H } {}

        size_t ticks;
        std::vector<uint8_t> audio;
        std::vector<uint8_t> video;
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

    const std::vector<frame> &get_frames() const { return frames_; }

    void compress( double stability, size_t byterate, bool group, const std::string &filters, const std::string &watermark )
    {
            //  The 3 possible compressor
        compressor<u_int8_t> c8{W_,H_};
        compressor<u_int16_t> c16{W_,H_};
        compressor<u_int32_t> c32{W_,H_};

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
            ::watermark( dest, watermark );
            framebuffer fb{ dest };

                //  Direct the 3 encoders to try this image
            c8.set_target_image( fb );
            c16.set_target_image( fb );
            c32.set_target_image( fb );

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
               
                    //  Encode within that budget with every codec
                std::array<std::vector<uint8_t>,3> encodes = 
                {
                    c8.next_tick( video_budget*.5 ),
                    c16.next_tick( video_budget ),
                    c32.next_tick( video_budget )
                };

                std::array<double,3> qualities = { c8.quality(), c16.quality(), c32.quality() };
                auto best_ix = std::max_element( std::begin(qualities), std::end(qualities) )-std::begin(qualities);

                if (qualities[1]>qualities[2])
                    best_ix = 1;
                else
                    best_ix = 2;

                // std::clog << " [" << qualities[0] << "," << qualities[1] << "," << qualities[2] << "]=";

                std::clog << best_ix;

                f.video = encodes[best_ix];
                f.video.insert( std::begin(f.video), best_ix );

                // f.video = c32.next_tick( video_budget );
                // f.video.insert( std::begin(f.video), 2 );

                f.video.insert( std::begin(f.video), 0x00 );
                f.video.insert( std::begin(f.video), 0x00 );
                f.video.insert( std::begin(f.video), 0x00 );

                    //  Add the current frame buffer
                if (best_ix==0)
                {
                    c16.set_current_image( c8.get_current_framebuffer() );
                    c32.set_current_image( c8.get_current_framebuffer() );
                    f.result = c8.get_current_framebuffer();
                }
                if (best_ix==1)
                {
                    c8.set_current_image( c16.get_current_framebuffer() );
                    c32.set_current_image( c16.get_current_framebuffer() );
                    f.result = c16.get_current_framebuffer();
                }
                if (best_ix==2)
                {
                    c8.set_current_image( c32.get_current_framebuffer() );
                    c16.set_current_image( c32.get_current_framebuffer() );
                    f.result = c32.get_current_framebuffer();
                }

                // f.result = c32.get_current_framebuffer();

                frames_.push_back( f );
            }

            // auto q = c.quality();

            auto q = c32.quality();

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


#endif
