#pragma once

#include <format>
#include "errors.hpp"
#include "grayscale.hpp"
#include "frame.hpp"
#include "imgcompress.hpp"
#include "compressor.hpp"

#include <vector>
#include <array>
#include <bitset>
#include <algorithm>
#include <memory>
#include <functional>
#include <optional>

using namespace macflim;

class encoding_profile;

#include "reader.hpp"
#include "subtitles.hpp"
#include "watermark.hpp"

#define VERBOSE

using namespace std::string_literals;

/**
 * The flimcompressor manages higher aspects of the compression
 */

class flimcompressor
{
private:
    size_t W_;
    size_t H_;

    std::function<std::optional<grayscale>()> next_image_;
    const std::vector<sound_frame_t> &audio_;
    const double fps_;
    std::vector<subtitle> subtitles_;

    std::vector<frame> frames_;
    std::optional<bitmap> initial_fb_;

public:
    flimcompressor(size_t W, size_t H, std::function<std::optional<grayscale>()> next_image, const std::vector<sound_frame_t> &audio, double fps, const std::vector<subtitle> &subtitles) : W_{W}, H_{H}, next_image_{std::move(next_image)}, audio_{audio}, fps_{fps}, subtitles_{subtitles} {}

    const std::vector<frame> &get_frames() const { return frames_; }
    const std::optional<bitmap> &get_initial() const { return initial_fb_; }

    bool progress_ = true;

    struct codec_spec
    {
        uint8_t signature;
        double penality;
        std::shared_ptr<compressor> coder;
    };

    static std::vector<std::string> split(const std::string s, const std::string delimiter)
    {
        size_t pos_start = 0, pos_end, delim_len = delimiter.length();
        std::string token;
        std::vector<std::string> res;

        while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos)
        {
            token = s.substr(pos_start, pos_end - pos_start);
            pos_start = pos_end + delim_len;
            res.push_back(token);
        }

        res.push_back(s.substr(pos_start));
        return res;
    }

    static codec_spec make_codec(const std::string &spec_string, size_t W, size_t H)
    {
        auto spec_array = split(spec_string, ":");
        auto name = spec_array[0];
        std::string parameters_string = "";

        if (spec_array.size() > 1)
            parameters_string = spec_array[1];

        codec_spec spec;
        spec.signature = 0x00;
        spec.penality = 1;
        spec.coder = std::make_shared<null_compressor>(W, H);

        if (name == "z16")
        {
            spec.signature = 0x01;
            spec.penality = 0.45;
            spec.coder = std::make_shared<vertical_compressor<uint16_t>>(W, H, uint16_ruler::ruler);
        }
        else if (name == "z32")
        {
            spec.signature = 0x02;
            spec.penality = 1.00;
            spec.coder = std::make_shared<vertical_compressor<uint32_t>>(W, H, uint32_ruler::ruler);
        }
        else if (name == "z32old")
        {
            static bit_ruler<uint32_t> br32;
            spec.signature = 0x02;
            spec.penality = 1.00;
            spec.coder = std::make_shared<vertical_compressor<uint32_t>>(W, H, br32);
        }
        else if (name == "invert")
        {
            spec.signature = 0x03;
            spec.penality = 1.00;
            spec.coder = std::make_shared<invert_compressor>(W, H);
        }
        else if (name == "lines")
        {
            spec.signature = 0x04;
            spec.penality = 1.00;
            spec.coder = std::make_shared<copy_line_compressor>(W, H);
        }
        else if (name == "null")
        {
            spec.signature = 0x00;
            spec.penality = 1.00;
            spec.coder = std::make_shared<null_compressor>(W, H);
        }
        else
        {
            std::clog << std::format("Unknown codec: [{}]\n", name);
            throw config_error("Unknown codec", name);
        }

        for (auto &param_string : split(parameters_string, ","))
        {
            auto v = split(param_string, "=");
            std::string pname = v[0];
            ;
            std::string pvalue = "";
            if (v.size() > 1)
                pvalue = v[1];
            spec.coder->set_parameter(pname, pvalue);
        }

        return spec;
    }

    struct DitheringParameters
    {
        const bool bars_;                   //  Do we add bars when we resize the added image?  (note: maybe do some grayscale normalizer class that does all conversion work)
        const std::string filters_;         //  Filters to apply
        const double anchor_x_;             //  Horizontal anchor for grayscale extraction
        const double anchor_y_;             //  Vertical anchor for grayscale extraction
        const grayscale::dithering dither_; //  The kind of dither to apply
        const std::string error_algorithm_; //  Error algo
        const double stability_;            //  Stability of the transform
        const float error_bleed_;
        const bool error_bidi_;
        const std::string watermark_; //  Unsure if this should be here or higher
    };

    /// This will dither a series of images, using the previous ones to minimize artifacts
    /// Size of the output is the same as the size of the initial image
    class Ditherer
    {
        size_t W_, H_;             //  Width and height of the generated image
        grayscale dithered_image_; //  The currently dithered image
                                   //  The initial grayscale defines the size of all future images

        const DitheringParameters dp_;

    public:
        Ditherer(const grayscale &initial_image, const DitheringParameters &dp) : W_{initial_image.W()},
                                                                                  H_{initial_image.H()},
                                                                                  dithered_image_{initial_image},
                                                                                  dp_{dp}
        {
        }

        size_t W() const { return W_; }
        size_t H() const { return H_; }

        /// Dither the grayscale according to the parameters
        /// Passed
        void dither(const grayscale &img)
        {
            grayscale resized_image(W_, H_); //  note: was 512x342
            copy(resized_image, img, dp_.bars_, dp_.anchor_x_, dp_.anchor_y_);

            //  We filter the grayscale of the "right size", for things like corners, etc...
            grayscale filtered_image = filter(resized_image, dp_.filters_.c_str());

            grayscale dithered_image(W_, H_); //  The next dithered image

            if (dp_.dither_ == grayscale::error_diffusion)
                error_diffusion(dithered_image, filtered_image, dithered_image_, dp_.stability_, *get_error_diffusion_by_name(dp_.error_algorithm_), dp_.error_bleed_, dp_.error_bidi_);
            else if (dp_.dither_ == grayscale::ordered)
                ordered_dither(dithered_image, filtered_image, dithered_image_);
            else if (dp_.dither_ == grayscale::blue_noise)
                blue_noise_dither(dithered_image, filtered_image, dithered_image_);
            else
                throw config_error("Unknown dithering option", std::to_string(dp_.dither_));

            ::watermark(dithered_image, dp_.watermark_);

            // char buffer[1024];
            // static int num = 0;
            // sprintf( buffer, "/tmp/foo-%04d.pgm", num );
            // num++;
            // write_grayscale( buffer, dithered_image );

            //  The new dithered grayscale is the previous one
            dithered_image_ = dithered_image;
        }

        //  The current dithered image
        const grayscale current()
        {
            return dithered_image_;
        }
    };

    class SubtitleBurner
    {
        std::vector<subtitle> subtitles_; //  The subtitles to burn
                                          // #### Should be a pair of const_iterators

    public:
        SubtitleBurner(const std::vector<subtitle> &subtitles) : subtitles_{subtitles}
        {
        }

        //  Burn the subtitle for time into the image;
        void burn_into(grayscale &img, double time)
        {
            if (subtitles_.size() > 0)
            {
                //  Can do that way better with an iterator!
                if (time >= subtitles_.front().start)
                {
                    if (time < subtitles_.front().stop)
                    {
                        ::burn_subtitle(img, subtitles_.front().text.front()); //  #### zero line subtitles will crash
                    }
                    else
                    {
                        subtitles_.erase(subtitles_.begin()); //  We should flip the subtitles order in constructor!
                    }
                }
            }
        }
    };

    class EncodingResult
    {
        const codec_spec &codec_;         //  Used codec
        bitmap image_;                    //  Resulting image
        const std::vector<uint8_t> data_; //  Resulting data
        const double quality_;            //  Resulting quality

    public:
        EncodingResult(
            const codec_spec &codec,
            const bitmap &current,
            const bitmap &target,
            const size_t budget) : codec_{codec},
                                   image_{current},
                                   data_{codec_.coder->compress(image_, target, budget * codec_.penality)},
                                   quality_{image_.proximity(target)}
        {
            // char buffer[1024];
            // static int num = 0;
            // sprintf( buffer, "/tmp/foo-%04d.pgm", num );
            // num++;
            // write_grayscale( buffer, image_.as_image() );
        }

        //  Encoded video with codec signature and trailer (#### why trailer?)
        std::vector<uint8_t> get_video_encoded_data() const
        {
            std::vector<uint8_t> result = {0x00, 0x00, 0x00, codec_.signature};
            result.insert(std::end(result), std::begin(data_), std::end(data_));
            return result;
        }

        double quality() const { return quality_; }
        const bitmap &image() const { return image_; }
    };

    class CompressorHelper
    {
        Ditherer &ditherer_;
        SubtitleBurner &subtitle_burner_;
        bitmap current_fb_; //  The bitmap displayed on screen at each step [#### check creation]
        const std::vector<codec_spec> &codecs_;
        const double fps_; //  Input fps
        const size_t byterate_;
        const std::vector<sound_frame_t> &audio_; //  The audio input
        bool group_;

        int in_fr_;                                                //  Input frame
        size_t current_tick_;                                      //  Output tick number
        std::vector<sound_frame_t>::const_iterator current_audio_; //  Current audio
        std::vector<frame> frames_;                                // Output generated frames
        bool log_progress_ = true;
        // double total_q_ = 0;        //  Total quality
        static const size_t BucketCount = 1000; //  Error distribution

        template <size_t N>
        class qhistogram
        {
            size_t total_ = 0;
            std::vector<size_t> samples_;
            bool verbose_ = true;

        public:
            qhistogram() : samples_(N + 1) {}

            ~qhistogram()
            {
                dump();
            }

            void add(double quality)
            {
                assert(quality >= 0 && quality <= 1);
                samples_[quality * N]++;
                total_++;
            }

            void dump() const
            {
                if (verbose_)
                {
                    std::clog << std::format("+----------+--------+----------+----------+\n");
                    std::clog << std::format("|     Q    | Frames |   Perc.  |  Cumul.  |\n");
                    std::clog << std::format("|----------|--------|----------|----------|\n");
                }

                size_t cumulative = 0;
                double var99 = 0;
                double var98 = 0;
                double var95 = 0;
                for (size_t i = 0; i != N + 1; i++)
                {
                    cumulative += samples_[i];
                    auto percent = (cumulative * 1.0 / total_);
                    if (percent > 0.01 && var99 == 0)
                        var99 = i * 1.0 / N;
                    if (percent > 0.02 && var98 == 0)
                        var98 = i * 1.0 / N;
                    if (percent > 0.05 && var95 == 0)
                        var95 = i * 1.0 / N;
                    if (verbose_)
                        if (samples_[i])
                            std::cerr << std::format("| {:7.3f}% | {:6} | {:7.3f}% | {:7.3f}% |\n",
                                                     i * 1.0 / N * 100, samples_[i],
                                                     (samples_[i] * 1.0 / total_) * 100, percent * 100);
                }
                if (verbose_)
                    std::clog << std::format("+----------+--------+----------+----------+\n");
                std::clog << std::format("99% of the frames are within {}% of the target pixels\n", var99 * 100);
                std::clog << std::format("98% of the frames are within {}% of the target pixels\n", var98 * 100);
                std::clog << std::format("95% of the frames are within {}% of the target pixels\n", var95 * 100);
            }
        };

        qhistogram<BucketCount> histo_;

    public:
        CompressorHelper(
            Ditherer &ditherer,
            SubtitleBurner &subtitle_burner,
            const std::vector<codec_spec> &codecs,
            const double fps,
            const size_t byterate,
            const std::vector<sound_frame_t> &audio,
            const bool group) : ditherer_{ditherer},
                                subtitle_burner_{subtitle_burner},
                                current_fb_{ditherer_.current()},
                                codecs_{codecs},
                                fps_{fps},
                                byterate_{byterate},
                                audio_{audio},
                                group_{group}
        {
            //  blah
            current_tick_ = 0;
            in_fr_ = 0;
            current_audio_ = std::begin(audio_);
        }

        // Adds one grayscale to the generated video, keep track of previous
        // Returns the quality metric (proximity to target)
        double add(const grayscale &source)
        {
            //  Dither the new image
            ditherer_.dither(source);
            grayscale dest = ditherer_.current();
            subtitle_burner_.burn_into(dest, in_fr_ / fps_);

            //  True B&W packed image
            bitmap fb{dest};

            //  Let's see how many ticks we have to display this image
            in_fr_++;
            size_t next_tick = ticks_from_frame(in_fr_, fps_);
            size_t ticks = next_tick - current_tick_;
            // std::clog << "current_tick:" << current_tick << " in_fr:" << in_fr << " next_tick:" << next_tick << " fps:" << fps_ << "\n";
            assert(ticks > 0);

            size_t local_ticks = 1;

            if (group_)
                local_ticks = ticks;

            for (size_t i = 0; i != ticks; i += local_ticks)
            {
                //  Add as much audio as we have for the local ticks
                std::vector<uint8_t> audio;
                for (size_t i = 0; i != local_ticks; i++)
                {
                    sound_frame_t snd;
                    if (current_audio_ < std::end(audio_))
                        snd = *current_audio_++;
                    std::copy(snd.begin(), snd.end(), std::back_inserter(audio));
                }

                // write_grayscale( "/tmp/a.pgm", fb.as_image() );
                // write_grayscale( "/tmp/b.pgm", dest );

                //  Compute the video budget?
                size_t video_budget = byterate_ * local_ticks;

                //  Encode within that budget with every codec
                std::vector<EncodingResult> encoding_results;
                std::transform(std::begin(codecs_), std::end(codecs_), std::back_inserter(encoding_results), [&](auto &codec) -> EncodingResult
                               { return EncodingResult(
                                     codec,
                                     current_fb_,
                                     fb,
                                     video_budget * codec.penality); });

                //  Find the result with highest quality
                auto best_result = std::max_element(encoding_results.begin(), encoding_results.end(), [](const EncodingResult &r1, const EncodingResult &r2)
                                                    { return r1.quality() < r2.quality(); });

                // write_grayscale( "/tmp/img1.pgm", best_result->image().as_image() );
                // exit(0);

                //  Construct the frame with best video and audio
                frame f{fb, local_ticks, best_result->get_video_encoded_data(), audio, best_result->image()};

                frames_.push_back(f);
                if (log_progress_)
                {
                    double time_s = current_tick_ / 60.0;
                    int min = (int)(time_s / 60);
                    double sec = time_s - min * 60;
                    std::cerr << std::format("Encoded frame {} ({}:{:05.2f}s)\r", frames_.size(), min, sec);
                }

                current_fb_ = best_result->image();
            }

            auto q = frames_.back().result->proximity(fb);

            // std::clog << "Q=" << q << " \n";

            // total_q_ += q;
            histo_.add(q);
            current_tick_ = next_tick;
            return q;
        }

        std::vector<frame> get_frames() const { return frames_; }
    };

    void compress(const encoding_profile &profile, const std::string &watermark, initial_frame_mode initial_mode = initial_frame_mode::optional, bool loop = false)
    {
        // Parse codec specs into codec objects
        std::vector<codec_spec> codecs;
        for (const auto &spec_str : profile.codec_specs())
        {
            codecs.push_back(make_codec(spec_str, W_, H_));
        }

        grayscale previous(W_, H_);
        fill(previous, 0);

        // Pull all images from the callback
        // We need the first grayscale for initial frame generation, so pull it now
        auto first_opt = next_image_();
        if (!first_opt)
        {
            std::clog << std::format("Warning: no input images\n");
            return;
        }

        // Create dithering parameters
        DitheringParameters dp{profile.bars(), profile.filters(), profile.anchor_x(), profile.anchor_y(), profile.dither(), profile.error_algorithm(), profile.stability(), profile.error_bleed(), profile.error_bidi(), watermark};

        bool process_first_image = true;

        // Generate initial frame if requested or if looping is enabled
        if (loop || initial_mode != initial_frame_mode::none)
        {
            // Create temporary Ditherer to generate initial frame
            Ditherer temp_d{previous, dp};
            temp_d.dither(*first_opt);
            initial_fb_ = bitmap{temp_d.current()};

            // For 'required' mode, start encoding from this image
            // For 'optional' mode, keep previous as black (backwards compatible)
            if (initial_mode == initial_frame_mode::required)
            {
                copy(previous, temp_d.current());
                process_first_image = false;
            }
        }

        // Create dithering infrastructure for encoding
        Ditherer d{previous, dp};
        SubtitleBurner sb{subtitles_};
        CompressorHelper ch{d, sb, codecs, fps_, profile.byterate(), audio_, profile.group()};

        // Process first image if not already used for initial frame
        if (process_first_image)
            ch.add(*first_opt);

        // Process remaining images from callback
        while (auto img = next_image_())
            ch.add(*img);

        // Perfect looping: add trailing frames until we return to initial frame
        if (loop && initial_fb_)
        {
            int trailing_count = 0;
            const int max_trailing = 100;
            double quality = 0.0;
            while (trailing_count < max_trailing && (quality = ch.add(*first_opt)) < 1.0)
            {
                trailing_count++;
            }
            if (quality < 1.0)
                std::clog << std::format("Warning: Loop did not achieve perfect quality after {} trailing frames (quality: {})\n",
                                         max_trailing, quality);
            else
                std::clog << std::format("Added {} trailing frames for perfect loop\n", trailing_count);
        }

        frames_ = ch.get_frames();
    }
};

#include "profile.hpp"
