#pragma once

#include <string>
#include <cstring>

#include "profile.hpp"
#include "flimformat.hpp"
#include "subtitles.hpp"

#include "reader.hpp"
#include "writer.hpp"

extern bool sDebug;

class flimencoder
{
    const encoding_profile &profile_;

    std::unique_ptr<output_writer> pgm_poster_writer_;
    std::unique_ptr<output_writer> pgm_diff_writer_;
    std::unique_ptr<output_writer> pgm_change_writer_;
    std::unique_ptr<output_writer> pgm_target_writer_;

    std::vector<subtitle> subtitles_;

    std::vector<image> images_;
    std::vector<sound_frame_t> audio_samples_;

    double fps_ = 24;
    double poster_ts_ = 0;

    std::string comment_;

    std::string watermark_;

    size_t cover_begin_; /// Begin index of cover image
    size_t cover_end_;   /// End index of cover image

    size_t frame_from_image(size_t n) const
    {
        return ticks_from_frame(n - 1, fps_ / profile_.fps_ratio());
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

        std::clog << "**** fps               : " << fps_ << "/" << profile_.fps_ratio() << "=" << fps_ / profile_.fps_ratio() << "\n";
        std::clog << "**** # of input images : " << images_.size() << "\n";
        std::clog << "**** # of movie ticks  : " << frame_from_image(images_.size() + 1) << "\n";
    }

    int clamp(double v, int a, int b)
    {
        int res = v + 0.5;
        if (res < a)
            res = a;
        if (res > b)
            res = b;
        return res;
    }

    std::vector<uint8_t> normalize_sound(std::vector<double> sound_samples, size_t len)
    {
        sound_samples.resize(len);
        std::vector<uint8_t> res;

        if (sound_samples.size() > 0)
        {
            auto mi = std::min_element(std::begin(sound_samples), std::end(sound_samples));
            auto ma = std::max_element(std::begin(sound_samples), std::end(sound_samples));
            double scale = std::max(::fabs(*mi), ::fabs(*ma));
            std::transform(std::begin(sound_samples), std::end(sound_samples), std::back_inserter(res), [&](double v)
                           { return clamp((v / scale) * 128 + 128, 0, 255); });
            std::clog << "Normalized  sound : [" << *mi << "," << *ma << "] => ["
                      << (int)*std::min_element(std::begin(res), std::end(res)) << ","
                      << (int)*std::max_element(std::begin(res), std::end(res)) << "]\n";
        }
        else
        {
            std::clog << "SOUND IS EMPTY\n";
        }

        return res;
    }

public:
    //  #### remove in and audio
    flimencoder(const encoding_profile &profile) : profile_{profile} {}

    void set_fps(double fps) { fps_ = fps; }
    void set_comment(const std::string comment) { comment_ = comment; }
    void set_cover(size_t cover_begin, size_t cover_end)
    {
        cover_begin_ = cover_begin;
        cover_end_ = cover_end;
    }
    void set_watermark(const std::string watermark) { watermark_ = watermark; }
    void set_pgm_poster_pattern(const std::string &pattern)
    {
        if (pattern != "")
            pgm_poster_writer_ = make_pgm_writer(pattern);
    }
    void set_pgm_diff_pattern(const std::string &pattern)
    {
        if (pattern != "")
            pgm_diff_writer_ = make_pgm_writer(pattern);
    }
    void set_pgm_change_pattern(const std::string &pattern)
    {
        if (pattern != "")
            pgm_change_writer_ = make_pgm_writer(pattern);
    }
    void set_pgm_target_pattern(const std::string &pattern)
    {
        if (pattern != "")
            pgm_target_writer_ = make_pgm_writer(pattern);
    }
    void set_poster_ts(double poster_ts) { poster_ts_ = poster_ts; }
    void set_subtitles(const std::vector<subtitle> &subtitles) { subtitles_ = subtitles; /* yes, it is a copy */ }

    //  Encode all the frames
    void make_flim(const std::string flim_pathname, input_reader *reader, const std::vector<std::unique_ptr<output_writer>> &writers)
    {
        assert(reader);

        int i = 0;
        while (auto next = reader->next())
        {
            if ((i % profile_.fps_ratio()) == 0)
                images_.push_back(*next);
            i++;
        }

        assert(images_.size() > 0);

        //  Poster extraction
        image poster_image = images_[0];
        size_t poster_index = poster_ts_ * fps_ / profile_.fps_ratio();

        if (poster_index < images_.size())
            poster_image = images_[poster_index];

        std::cout << "POSTER INDEX: " << poster_index << "\n";

        auto filters_string = profile_.filters();
        poster_image = filter(poster_image, filters_string.c_str());

        image poster_small(128, 86);
        copy(poster_small, poster_image, false, 0.5, 0.5);

        image previous(poster_small.W(), poster_small.H());
        fill(previous, 0);

        // auto prev = poster_small;
        auto poster_small_bw = poster_small;
        // auto error_diff = get_error_diffusion_by_name( "floyd" );

        if (profile_.dither() == image::error_diffusion)
            error_diffusion(poster_small_bw, poster_small, previous, 0, *get_error_diffusion_by_name(profile_.error_algorithm()), profile_.error_bleed(), profile_.error_bidi());
        else if (profile_.dither() == image::ordered)
            ordered_dither(poster_small_bw, poster_small, previous);
        else if (profile_.dither() == image::blue_noise)
            blue_noise_dither(poster_small_bw, poster_small, previous);

        // error_diffusion( poster_small_bw, poster_small, prev, 0, *error_diff, 0.99, true );
        write_image("/tmp/poster1.pgm", poster_image);
        write_image("/tmp/poster2.pgm", poster_small);
        write_image("/tmp/poster3.pgm", poster_small_bw);

        if (!profile_.silent())
            while (auto next = reader->next_sound())
            {
                audio_samples_.push_back(*next);
            }

        // audio_samples_ = normalize_sound( reader->raw_sound(), images_.size()/fps_*60*370 );

        fix();

        flimcompressor fc{profile_.width(), profile_.height(), images_, audio_samples_, fps_ / profile_.fps_ratio(), subtitles_};

        fc.compress(profile_.stability(), profile_.byterate(), profile_.group(), profile_.filters(), watermark_, profile_.codecs(), profile_.dither(), profile_.bars(), profile_.anchor_x(), profile_.anchor_y(), profile_.error_algorithm(), profile_.error_bleed(), profile_.error_bidi(), profile_.initial_mode(), profile_.loop());

        auto frames = fc.get_frames();

        // Diagnostic PGM generation - poster thumbnails from original images
        if (pgm_poster_writer_)
        {
            for (auto &poster_source : images_)
            {
                image poster_small(128, 86);
                copy(poster_small, poster_source, false);

                auto prev = poster_small;
                auto poster_small_bw = poster_small;
                auto error_diff = get_error_diffusion_by_name("floyd");

                error_diffusion(poster_small_bw, poster_small, prev, 0, *error_diff, 0.99, true);
                pgm_poster_writer_->write_frame(poster_small_bw, {});
            }
        }

        // Diagnostic PGM generation - frame analysis
        if (pgm_diff_writer_ || pgm_change_writer_ || pgm_target_writer_)
        {
            framebuffer previous_frame{profile_.width(), profile_.height()};
            previous_frame.fill(0xff);

            for (auto &frame : frames)
            {
                if (pgm_diff_writer_)
                {
                    auto logimg = (frame.result ^ frame.source).inverted().as_image();
                    pgm_diff_writer_->write_frame(logimg, {});
                }
                if (pgm_change_writer_)
                {
                    auto logimg = (frame.result ^ previous_frame).inverted().as_image();
                    pgm_change_writer_->write_frame(logimg, {});
                    previous_frame = frame.result;
                }
                if (pgm_target_writer_)
                {
                    auto logimg = frame.source.as_image();
                    pgm_target_writer_->write_frame(logimg, {});
                }
            }
        }

        // Generate FLIM file
        encoded_flim ef{comment_};
        size_t total_ticks = std::accumulate(std::begin(frames), std::end(frames), 0, [](size_t a, const flimcompressor::frame &f)
                                             { return a + f.ticks; });
        flim_info fi{profile_.width(), profile_.height(), profile_.silent(), frames.size(), total_ticks, profile_.byterate()};
        ef.add(fi);
        ef.add(frames);
        ef.add_poster(poster_small_bw);

        // Add initial frame if generated
        if (fc.get_initial())
            ef.add_initial(*fc.get_initial());

        FILE *movie_file = fopen(flim_pathname.c_str(), "wb");
        ef.fwrite(movie_file);
        fclose(movie_file);

        // Production output via writers (mp4, gif, pgm)
        if (writers.size())
        {
            for (auto &writer : writers)
            {
                auto sound = std::begin(audio_samples_);

                for (auto &frame : frames)
                {
                    for (size_t i = 0; i != frame.ticks; i++)
                    {
                        sound_frame_t snd;
                        if (!profile_.silent())
                            if (sound < std::end(audio_samples_))
                                snd = *sound++;
                        writer->write_frame(frame.result.as_image(), snd);
                    }
                }
            }
        }

        // Cover generation
        for (size_t i = cover_begin_; i <= cover_end_; i++)
        {
            if (i < frames.size())
            {
                char buffer[1024];
                std::clog << "COVER " << i << "\n";
                sprintf(buffer, "cover-%06zu.pgm", i - cover_begin_ + 1);
                auto logimg = frames[i].result.as_image();
                write_image(buffer, logimg);
            }
        }
    }
};
