#pragma once

#include "grayscale.hpp"
#include "flimcompressor.hpp"
#include <sstream>
#include <functional>

using namespace std::string_literals;

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
    bool bars_ = true;      //  Do we put black bars around the image?
    double anchor_x_ = 0.5; //  Horizontal anchor: 0=left, 0.5=center, 1=right
    double anchor_y_ = 0.5; //  Vertical anchor: 0=top, 0.5=center, 1=bottom

    grayscale::dithering dither_ = grayscale::error_diffusion;
    std::string error_algorithm_ = "floyd";
    float error_bleed_ = 1;
    bool error_bidi_ = false;

    bool silent_ = false;

    initial_frame_mode initial_mode_ = initial_frame_mode::optional;
    bool loop_ = false;

    std::vector<flimcompressor::codec_spec> codecs_;
    std::function<std::vector<flimcompressor::codec_spec>(const encoding_profile &)> codec_factory_ =
        [](const encoding_profile &)
    { return std::vector<flimcompressor::codec_spec>(); };

public:
    size_t width() const { return W_; }
    size_t height() const { return H_; }
    void set_size(size_t W, size_t H)
    {
        W_ = W;
        H_ = H;
    }
    void set_width(size_t W) { W_ = W; }
    void set_height(size_t H) { H_ = H; }

    size_t byterate() const { return byterate_; }
    void set_byterate(size_t byterate) { byterate_ = byterate; }

    //  Technically, we could put the half-rate/fps_ratio mecanism in the reader phase
    //  to avoid reading unecessary images, but it is more generic to put it here
    //  as it could allows to extend to dynamic half rate [yagni]
    int fps_ratio() const { return fps_ratio_; }
    void set_fps_ratio(int fps_ratio) { fps_ratio_ = fps_ratio; }

    bool group() const { return group_; }
    void set_group(bool group) { group_ = group; }

    std::string filters() const { return filters_; }
    void set_filters(const std::string filters) { filters_ = filters; }

    bool bars() const { return bars_; }
    void set_bars(bool bars) { bars_ = bars; }

    double anchor_x() const { return anchor_x_; }
    void set_anchor_x(double anchor_x) { anchor_x_ = anchor_x; }

    double anchor_y() const { return anchor_y_; }
    void set_anchor_y(double anchor_y) { anchor_y_ = anchor_y; }

    grayscale::dithering dither() const { return dither_; }
    bool set_dither(std::string dither)
    {
        if (dither == "ordered")
            dither_ = grayscale::ordered;
        else if (dither == "error")
            dither_ = grayscale::error_diffusion;
        else if (dither == "blue")
            dither_ = grayscale::blue_noise;
        else
            throw "Wrong dither option : only 'ordered', 'error', and 'blue' are supported";
        return true;
    }
    void set_dither(grayscale::dithering dither) { dither_ = dither; }

    std::string error_algorithm() const { return error_algorithm_; }
    void set_error_algorithm(const std::string algo) { error_algorithm_ = algo; }

    float error_bleed() const { return error_bleed_; }
    void set_error_bleed(float bleed) { error_bleed_ = bleed; }

    bool error_bidi() const { return error_bidi_; }
    void set_error_bidi(bool error_bidi) { error_bidi_ = error_bidi; }

    double stability() const { return stability_; }
    void set_stability(double stability) { stability_ = stability; }

    const std::vector<flimcompressor::codec_spec> &codecs() const { return codecs_; }
    void set_codecs(const std::vector<flimcompressor::codec_spec> &codecs) { codecs_ = codecs; }

    void create_codecs()
    {
        set_codecs(codec_factory_(*this));
    }

    bool silent() const { return silent_; }
    void set_silent(bool silent) { silent_ = silent; }

    initial_frame_mode initial_mode() const { return initial_mode_; }
    void set_initial_mode(initial_frame_mode mode) { initial_mode_ = mode; }
    bool set_initial_mode(const std::string &mode)
    {
        if (mode == "false" || mode == "none")
            initial_mode_ = initial_frame_mode::none;
        else if (mode == "optional")
            initial_mode_ = initial_frame_mode::optional;
        else if (mode == "true" || mode == "required")
            initial_mode_ = initial_frame_mode::required;
        else
            return false;
        return true;
    }

    bool loop() const { return loop_; }
    void set_loop(bool loop) { loop_ = loop; }

    static bool profile_named(const std::string name, encoding_profile &result)
    {
        if (name == "128k"s)
        {
            result.set_size(512, 342);
            result.set_byterate(380);
            result.set_filters("g1.6bbscz");
            result.set_fps_ratio(4);
            result.set_group(false);
            result.set_stability(0.5);
            result.set_bars(true);
            result.set_dither("ordered");
            result.set_error_algorithm("floyd");
            result.set_error_bidi(true);
            result.set_error_bleed(0.95);
            result.codecs_.clear();
            result.codecs_.push_back(flimcompressor::make_codec("null", result.W_, result.H_));
            result.codecs_.push_back(flimcompressor::make_codec("z32", result.W_, result.H_));
            result.codecs_.push_back(flimcompressor::make_codec("lines:count=10", result.W_, result.H_));
            result.codecs_.push_back(flimcompressor::make_codec("invert", result.W_, result.H_));
            result.set_silent(true);
            return true;
        }
        if (name == "512k"s)
        {
            result.set_size(512, 342);
            result.set_byterate(480);
            result.set_filters("g1.6bbscz");
            result.set_fps_ratio(4);
            result.set_group(false);
            result.set_stability(0.5);
            result.set_bars(true);
            result.set_dither("ordered");
            result.set_error_algorithm("floyd");
            result.set_error_bidi(true);
            result.set_error_bleed(0.95);
            result.codec_factory_ = [](const encoding_profile &p)
            {
                std::vector<flimcompressor::codec_spec> codecs;
                codecs.push_back(flimcompressor::make_codec("null", p.width(), p.height()));
                codecs.push_back(flimcompressor::make_codec("z32", p.width(), p.height()));
                codecs.push_back(flimcompressor::make_codec("lines:count=10", p.width(), p.height()));
                codecs.push_back(flimcompressor::make_codec("invert", p.width(), p.height()));
                return codecs;
            };
            result.set_silent(true);
            return true;
        }
        if (name == "xl"s)
        {
            result.set_size(704, 364);
            result.set_byterate(580);
            result.set_filters("g1.6bbsc");
            result.set_fps_ratio(4);
            result.set_group(true);
            result.set_stability(0.5);
            result.set_bars(true);
            result.set_dither("ordered");
            result.set_error_algorithm("floyd");
            result.set_error_bidi(true);
            result.set_error_bleed(0.95);
            result.codec_factory_ = [](const encoding_profile &p)
            {
                std::vector<flimcompressor::codec_spec> codecs;
                codecs.push_back(flimcompressor::make_codec("null", p.width(), p.height()));
                codecs.push_back(flimcompressor::make_codec("z32", p.width(), p.height()));
                codecs.push_back(flimcompressor::make_codec("lines:count=50", p.width(), p.height()));
                codecs.push_back(flimcompressor::make_codec("invert", p.width(), p.height()));
                return codecs;
            };
            result.set_silent(true);
            return true;
        }
        if (name == "plus"s)
        {
            result.set_size(512, 342);
            result.set_byterate(1500);
            result.set_filters("g1.6bbscz");
            result.set_fps_ratio(2);
            result.set_group(false);
            result.set_stability(0.5);
            result.set_bars(true);
            result.set_dither("ordered");
            result.set_error_algorithm("floyd");
            result.set_error_bidi(true);
            result.set_error_bleed(0.95);
            result.codec_factory_ = [](const encoding_profile &p)
            {
                std::vector<flimcompressor::codec_spec> codecs;
                codecs.push_back(flimcompressor::make_codec("null", p.width(), p.height()));
                codecs.push_back(flimcompressor::make_codec("z32", p.width(), p.height()));
                codecs.push_back(flimcompressor::make_codec("lines:count=30", p.width(), p.height()));
                codecs.push_back(flimcompressor::make_codec("invert", p.width(), p.height()));
                return codecs;
            };
            result.set_silent(false);
            return true;
        }
        //  The MicroMac Performer accelerator (16MHz 68030 on a plus)
        if (name == "performer"s)
        {
            result.set_size(512, 342);
            result.set_byterate(5000);
            result.set_filters("g1.6bsc");
            result.set_fps_ratio(2);
            result.set_group(false);
            result.set_stability(0.5);
            result.set_bars(true);
            result.set_dither("blue");
            result.set_error_algorithm("floyd");
            result.set_error_bidi(true);
            result.set_error_bleed(0.95);
            result.codec_factory_ = [](const encoding_profile &p)
            {
                std::vector<flimcompressor::codec_spec> codecs;
                codecs.push_back(flimcompressor::make_codec("null", p.width(), p.height()));
                codecs.push_back(flimcompressor::make_codec("z32", p.width(), p.height()));
                codecs.push_back(flimcompressor::make_codec("lines:count=30", p.width(), p.height()));
                codecs.push_back(flimcompressor::make_codec("invert", p.width(), p.height()));
                return codecs;
            };
            result.set_silent(false);
            return true;
        }
        if (name == "portable"s)
        {
            result.set_size(640, 400);
            result.set_byterate(2500);
            result.set_filters("g1.6bsc");
            result.set_fps_ratio(2);
            result.set_group(false);
            result.set_stability(0.5);
            result.set_bars(true);
            result.set_dither("error");
            result.set_error_algorithm("floyd");
            result.set_error_bidi(true);
            result.set_error_bleed(0.98);
            result.codec_factory_ = [](const encoding_profile &p)
            {
                std::vector<flimcompressor::codec_spec> codecs;
                codecs.push_back(flimcompressor::make_codec("null", p.width(), p.height()));
                codecs.push_back(flimcompressor::make_codec("z32", p.width(), p.height()));
                codecs.push_back(flimcompressor::make_codec("lines:count=50", p.width(), p.height()));
                codecs.push_back(flimcompressor::make_codec("invert", p.width(), p.height()));
                return codecs;
            };
            result.set_silent(false);
            return true;
        }
        if (name == "se"s)
        {
            result.set_size(512, 342);
            result.set_byterate(2500);
            result.set_filters("g1.6bsc");
            result.set_fps_ratio(2);
            result.set_group(false);
            result.set_stability(0.5);
            result.set_bars(true);
            result.set_dither("error");
            result.set_error_algorithm("floyd");
            result.set_error_bidi(true);
            result.set_error_bleed(0.98);
            result.codec_factory_ = [](const encoding_profile &p)
            {
                std::vector<flimcompressor::codec_spec> codecs;
                codecs.push_back(flimcompressor::make_codec("null", p.width(), p.height()));
                codecs.push_back(flimcompressor::make_codec("z32", p.width(), p.height()));
                codecs.push_back(flimcompressor::make_codec("lines:count=50", p.width(), p.height()));
                codecs.push_back(flimcompressor::make_codec("invert", p.width(), p.height()));
                return codecs;
            };
            result.set_silent(false);
            return true;
        }
        if (name == "se30"s)
        {
            result.set_size(512, 342);
            result.set_byterate(6000);
            result.set_filters("g1.6sc");
            result.set_fps_ratio(1);
            result.set_group(true);
            result.set_stability(0.3);
            result.set_bars(false);
            result.set_dither("error");
            result.set_error_algorithm("floyd");
            result.set_error_bidi(true);
            result.set_error_bleed(0.99);
            result.codec_factory_ = [](const encoding_profile &p)
            {
                std::vector<flimcompressor::codec_spec> codecs;
                codecs.push_back(flimcompressor::make_codec("null", p.width(), p.height()));
                codecs.push_back(flimcompressor::make_codec("z32", p.width(), p.height()));
                codecs.push_back(flimcompressor::make_codec("lines:count=70", p.width(), p.height()));
                codecs.push_back(flimcompressor::make_codec("invert", p.width(), p.height()));
                return codecs;
            };
            result.set_silent(false);
            return true;
        }
        if (name == "perfect"s)
        {
            result.set_size(512, 342);
            result.set_byterate(32000);
            result.set_filters("g1.6sc");
            result.set_fps_ratio(1);
            result.set_group(true);
            result.set_stability(0.3);
            result.set_bars(false);
            result.set_dither("error");
            result.set_error_algorithm("floyd");
            result.set_error_bidi(true);
            result.set_error_bleed(1);
            result.codec_factory_ = [](const encoding_profile &p)
            {
                std::vector<flimcompressor::codec_spec> codecs;
                codecs.push_back(flimcompressor::make_codec("null", p.width(), p.height()));
                codecs.push_back(flimcompressor::make_codec("z32", p.width(), p.height()));
                codecs.push_back(flimcompressor::make_codec("lines:count=70", p.width(), p.height()));
                codecs.push_back(flimcompressor::make_codec("invert", p.width(), p.height()));
                return codecs;
            };
            result.set_silent(false);
            return true;
        }

        return false;
    }

    std::string dither_string() const
    {
        switch (dither_)
        {
        case grayscale::error_diffusion:
            return "error";
        case grayscale::ordered:
            return "ordered";
        case grayscale::blue_noise:
            return "blue";
        }
        return "???";
    }

    std::string description() const
    {
        std::ostringstream cmd;

        cmd << "--byterate " << byterate_;
        cmd << " --fps-ratio " << fps_ratio_;
        cmd << " --group " << (group_ ? "true" : "false");
        cmd << " --bars " << (bars_ ? "true" : "false");
        cmd << " --dither " << dither_string();
        if (dither_ == grayscale::error_diffusion)
        {
            cmd << " --error-stability " << stability_;
            cmd << " --error-algorithm " << error_algorithm_;
            cmd << " --error-bidi " << error_bidi_;
            cmd << " --error-bleed " << error_bleed_;
        }
        cmd << " --filters " << filters_;

        for (auto &c : codecs_)
            cmd << " --codec " << c.coder->description();

        cmd << " --silent " << (silent_ ? "true" : "false");

        // Add initial-frame mode to description
        const char *initial_mode_str = "optional";
        if (initial_mode_ == initial_frame_mode::none)
            initial_mode_str = "false";
        else if (initial_mode_ == initial_frame_mode::required)
            initial_mode_str = "true";
        cmd << " --initial-frame " << initial_mode_str;

        cmd << " --loop " << (loop_ ? "true" : "false");

        return cmd.str();
    }
};
