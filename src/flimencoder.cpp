#include "flimencoder.hpp"

#include <cassert>
#include <format>
#include <numeric>

#include "bitmap.hpp"
#include "flim.hpp"
#include "flimcompressor.hpp"
#include "frame.hpp"

namespace macflim
{

int flimencoder::clamp(double v, int a, int b)
{
    int res = v + 0.5;
    if (res < a)
        res = a;
    if (res > b)
        res = b;
    return res;
}

std::vector<uint8_t> flimencoder::normalize_sound(std::vector<double> sound_samples, size_t len)
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
        std::clog << std::format("Normalized sound: [{},{}] => [{},{}]\n",
                                 *mi, *ma,
                                 (int)*std::min_element(std::begin(res), std::end(res)),
                                 (int)*std::max_element(std::begin(res), std::end(res)));
    }
    else
    {
        std::clog << std::format("SOUND IS EMPTY\n");
    }

    return res;
}

void flimencoder::set_fps(double fps) { fps_ = fps; }
void flimencoder::set_comment(const std::string comment) { comment_ = comment; }
void flimencoder::set_cover(size_t cover_begin, size_t cover_end)
{
    cover_begin_ = cover_begin;
    cover_end_ = cover_end;
}
void flimencoder::set_watermark(const std::string watermark) { watermark_ = watermark; }
void flimencoder::set_pgm_poster_pattern(const std::string &pattern)
{
    if (pattern != "")
        pgm_poster_writer_ = make_pgm_writer(pattern);
}
void flimencoder::set_pgm_diff_pattern(const std::string &pattern)
{
    if (pattern != "")
        pgm_diff_writer_ = make_pgm_writer(pattern);
}
void flimencoder::set_pgm_change_pattern(const std::string &pattern)
{
    if (pattern != "")
        pgm_change_writer_ = make_pgm_writer(pattern);
}
void flimencoder::set_pgm_target_pattern(const std::string &pattern)
{
    if (pattern != "")
        pgm_target_writer_ = make_pgm_writer(pattern);
}
void flimencoder::set_poster_ts(double poster_ts) { poster_ts_ = poster_ts; }
void flimencoder::set_subtitles(const std::vector<subtitle> &subtitles) { subtitles_ = subtitles; /* yes, it is a copy */ }

//  Encode all the frames
void flimencoder::make_flim(const std::string flim_pathname, input_reader *reader, std::vector<sound_frame_t> audio_samples, const std::vector<std::unique_ptr<output_writer>> &writers)
{
    assert(reader);

    // Build a pull-callback that wraps the reader, applies fps_ratio skipping,
    // and captures the poster grayscale at the right index.
    size_t poster_index = poster_ts_ * fps_ / profile_.fps_ratio();
    std::clog << std::format("POSTER INDEX: {}\n", poster_index);

    size_t image_count = 0;
    grayscale poster_image(1, 1); // Will be overwritten during the sequential pass
    bool poster_captured = false;
    int raw_frame_index = 0;

    auto next_image = [&]() -> std::optional<grayscale>
    {
        while (true)
        {
            auto next = reader->next();
            if (!next)
                return std::nullopt;

            // fps_ratio skipping: keep every Nth frame
            int idx = raw_frame_index++;
            if ((idx % profile_.fps_ratio()) != 0)
                continue;

            // Capture poster during the sequential pass
            if (image_count == poster_index || (!poster_captured && image_count == 0))
            {
                poster_image = *next;
                poster_captured = true;
            }

            image_count++;
            return *next;
        }
    };

    // Peek the first grayscale to ensure we have at least one frame
    auto first = next_image();
    assert(first.has_value());

    // If poster is frame 0, it's already captured. Wrap in a callback that
    // yields the first grayscale first, then continues pulling.
    bool first_consumed = false;
    auto next_image_with_first = [&]() -> std::optional<grayscale>
    {
        if (!first_consumed)
        {
            first_consumed = true;
            return *first;
        }
        return next_image();
    };

    std::clog << std::format("**** fps: {}/{}={}\n",
                             fps_, profile_.fps_ratio(), fps_ / profile_.fps_ratio());

    //  Poster processing
    auto filters_string = profile_.filters();
    grayscale poster_filtered = filter(poster_image, filters_string.c_str());

    grayscale poster_small(128, 86);
    copy(poster_small, poster_filtered, false, 0.5, 0.5);

    grayscale previous(poster_small.W(), poster_small.H());
    fill(previous, 0);

    auto poster_small_bw = poster_small;

    if (profile_.dither() == grayscale::error_diffusion)
        error_diffusion(poster_small_bw, poster_small, previous, 0, *get_error_diffusion_by_name(profile_.error_algorithm()), profile_.error_bleed(), profile_.error_bidi());
    else if (profile_.dither() == grayscale::ordered)
        ordered_dither(poster_small_bw, poster_small, previous);
    else if (profile_.dither() == grayscale::blue_noise)
        blue_noise_dither(poster_small_bw, poster_small, previous);

    write_grayscale("/tmp/poster1.pgm", poster_filtered);
    write_grayscale("/tmp/poster2.pgm", poster_small);
    write_grayscale("/tmp/poster3.pgm", poster_small_bw);

    // Compress
    flimcompressor fc{profile_.width(), profile_.height(), next_image_with_first, audio_samples, fps_ / profile_.fps_ratio(), subtitles_};

    fc.compress(profile_, watermark_, profile_.initial_mode(), profile_.loop());

    auto frames = fc.get_frames();

    std::clog << std::format("\n**** # of input images: {}\n", image_count);

    // Diagnostic PGM generation - frame analysis
    // (pgm_poster_writer_ is no longer supported without images_ vector)
    if (pgm_diff_writer_ || pgm_change_writer_ || pgm_target_writer_)
    {
        bitmap previous_frame{profile_.width(), profile_.height()};
        previous_frame.fill(0xff);

        for (auto &frame : frames)
        {
            if (pgm_diff_writer_)
            {
                auto logimg = (*frame.result ^ *frame.source).inverted().as_image();
                pgm_diff_writer_->write_frame(logimg, {});
            }
            if (pgm_change_writer_)
            {
                auto logimg = (*frame.result ^ previous_frame).inverted().as_image();
                pgm_change_writer_->write_frame(logimg, {});
                previous_frame = *frame.result;
            }
            if (pgm_target_writer_)
            {
                auto logimg = frame.source->as_image();
                pgm_target_writer_->write_frame(logimg, {});
            }
        }
    }

    // Generate FLIM file
    flim ef{comment_};
    size_t total_ticks = std::accumulate(std::begin(frames), std::end(frames), 0, [](size_t a, const frame &f)
                                         { return a + f.ticks; });
    flim_info fi{profile_.width(), profile_.height(), profile_.silent(), frames.size(), total_ticks, profile_.byterate()};
    ef.add(fi);
    ef.add(frames);
    ef.add_poster(poster_small_bw);

    // Add initial frame if generated
    if (fc.get_initial())
        ef.add_initial(*fc.get_initial());

    FILE *movie_file = fopen(flim_pathname.c_str(), "wb");
    ef.write(movie_file);
    fclose(movie_file);

    // Production output via writers (mp4, gif, pgm)
    if (writers.size())
    {
        size_t total_ticks = 0;
        for (auto &frame : frames)
            total_ticks += frame.ticks;

        for (auto &writer : writers)
        {
            auto sound = std::begin(audio_samples);
            size_t tick_count = 0;

            for (auto &frame : frames)
            {
                for (size_t i = 0; i != frame.ticks; i++)
                {
                    sound_frame_t snd;
                    if (!profile_.silent())
                        if (sound < std::end(audio_samples))
                            snd = *sound++;
                    writer->write_frame(frame.result->as_image(), snd);
                    tick_count++;
                    if (tick_count % 60 == 0 || tick_count == total_ticks)
                    {
                        double secs = tick_count / 60.0;
                        int mins = (int)(secs / 60);
                        double rsecs = secs - mins * 60;
                        std::cerr << std::format("Writing output: tick {}/{} ({}:{:05.2f}s) {}%\r",
                                                 tick_count, total_ticks, mins, rsecs,
                                                 tick_count * 100 / total_ticks);
                    }
                }
            }
            std::cerr << std::format("\n");
        }
    }

    // Cover generation
    for (size_t i = cover_begin_; i <= cover_end_; i++)
    {
        if (i < frames.size())
        {
            std::clog << std::format("COVER {}\n", i);
            std::string buffer = std::format("cover-{:06}.pgm", i - cover_begin_ + 1);
            auto logimg = frames[i].result->as_image();
            write_grayscale(buffer.c_str(), logimg);
        }
    }
}

} // namespace macflim
