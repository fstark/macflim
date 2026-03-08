#pragma once

#include "constants.hpp"
#include "errors.hpp"
#include "grayscale.hpp"

#include <sstream>
#include <string>
#include <vector>

namespace macflim
{

enum class initial_frame_mode
{
    none,     // No initial frame generated
    optional, // Generate initial frame but don't use for encoding
    required  // Generate and use initial frame for first comparison
};

/**
 * A set of encoding parameters
 */
class encoding_profile
{
  protected:
    size_t W_ = constants::mac_screen_width;
    size_t H_ = constants::mac_screen_height;

    size_t byterate_ = 2000;
    double stability_ = 0.3;
    int fps_ratio_ = 1;
    bool group_ = true;
    std::string filters_ = "c";
    bool bars_ = true;      //  Do we put black bars around the image?
    double anchor_x_ = 0.5; //  Horizontal anchor: 0=left, 0.5=center, 1=right
    double anchor_y_ = 0.5; //  Vertical anchor: 0=top, 0.5=center, 1=bottom

    grayscale::dithering dither_ = grayscale::dithering::error_diffusion;
    std::string error_algorithm_ = "floyd";
    float error_bleed_ = 1;
    bool error_bidi_ = false;

    bool silent_ = false;

    initial_frame_mode initial_mode_ = initial_frame_mode::optional;
    bool loop_ = false;

    std::vector<std::string> codec_specs_;

  public:
    [[nodiscard]] size_t width() const
    {
        return W_;
    }
    [[nodiscard]] size_t height() const
    {
        return H_;
    }
    void set_size(size_t W, size_t H)
    {
        W_ = W;
        H_ = H;
    }
    void set_width(size_t W)
    {
        W_ = W;
    }
    void set_height(size_t H)
    {
        H_ = H;
    }

    [[nodiscard]] size_t byterate() const
    {
        return byterate_;
    }
    void set_byterate(size_t byterate)
    {
        byterate_ = byterate;
    }

    //  Technically, we could put the half-rate/fps_ratio mecanism in the reader phase
    //  to avoid reading unecessary images, but it is more generic to put it here
    //  as it could allows to extend to dynamic half rate [yagni]
    [[nodiscard]] int fps_ratio() const
    {
        return fps_ratio_;
    }
    void set_fps_ratio(int fps_ratio)
    {
        fps_ratio_ = fps_ratio;
    }

    [[nodiscard]] bool group() const
    {
        return group_;
    }
    void set_group(bool group)
    {
        group_ = group;
    }

    [[nodiscard]] std::string filters() const
    {
        return filters_;
    }
    void set_filters(const std::string filters)
    {
        filters_ = filters;
    }

    [[nodiscard]] bool bars() const
    {
        return bars_;
    }
    void set_bars(bool bars)
    {
        bars_ = bars;
    }

    [[nodiscard]] double anchor_x() const
    {
        return anchor_x_;
    }
    void set_anchor_x(double anchor_x)
    {
        anchor_x_ = anchor_x;
    }

    [[nodiscard]] double anchor_y() const
    {
        return anchor_y_;
    }
    void set_anchor_y(double anchor_y)
    {
        anchor_y_ = anchor_y;
    }

    [[nodiscard]] grayscale::dithering dither() const
    {
        return dither_;
    }
    void set_dither(std::string dither);
    void set_dither(grayscale::dithering dither)
    {
        dither_ = dither;
    }

    [[nodiscard]] std::string error_algorithm() const
    {
        return error_algorithm_;
    }
    void set_error_algorithm(const std::string algo)
    {
        error_algorithm_ = algo;
    }

    [[nodiscard]] float error_bleed() const
    {
        return error_bleed_;
    }
    void set_error_bleed(float bleed)
    {
        error_bleed_ = bleed;
    }

    [[nodiscard]] bool error_bidi() const
    {
        return error_bidi_;
    }
    void set_error_bidi(bool error_bidi)
    {
        error_bidi_ = error_bidi;
    }

    [[nodiscard]] double stability() const
    {
        return stability_;
    }
    void set_stability(double stability)
    {
        stability_ = stability;
    }

    [[nodiscard]] const std::vector<std::string> &codec_specs() const
    {
        return codec_specs_;
    }
    void set_codec_specs(const std::vector<std::string> &specs)
    {
        codec_specs_ = specs;
    }

    [[nodiscard]] bool silent() const
    {
        return silent_;
    }
    void set_silent(bool silent)
    {
        silent_ = silent;
    }

    [[nodiscard]] initial_frame_mode initial_mode() const
    {
        return initial_mode_;
    }
    void set_initial_mode(initial_frame_mode mode)
    {
        initial_mode_ = mode;
    }
    void set_initial_mode(const std::string &mode);

    [[nodiscard]] bool loop() const
    {
        return loop_;
    }
    void set_loop(bool loop)
    {
        loop_ = loop;
    }

  private:
    static constexpr size_t MAX_CODECS = 8;

    struct profile_config
    {
        const char *name;
        size_t width, height;
        size_t byterate;
        const char *filters;
        int fps_ratio;
        bool group;
        double stability;
        bool bars;
        const char *dither;
        const char *error_algorithm;
        bool error_bidi;
        float error_bleed;
        bool silent;
        const char *codecs[MAX_CODECS];
    };

    static constexpr profile_config profile_table[] = {
        //  name       w    h     rate  filters   ratio  grp   stab  bars   dithering  error    bidi   bleed  silent
        //  codecs
        {"128k",
         512,
         342,
         380,
         "g1.6bbscz",
         4,
         false,
         0.5,
         true,
         "ordered",
         "floyd",
         true,
         0.95f,
         true,
         {"null", "z32", "lines:count=10", "invert"}},
        {"512k",
         512,
         342,
         480,
         "g1.6bbscz",
         4,
         false,
         0.5,
         true,
         "ordered",
         "floyd",
         true,
         0.95f,
         true,
         {"null", "z32", "lines:count=10", "invert"}},
        {"xl",
         704,
         364,
         580,
         "g1.6bbsc",
         4,
         true,
         0.5,
         true,
         "ordered",
         "floyd",
         true,
         0.95f,
         true,
         {"null", "z32", "lines:count=50", "invert"}},
        {"plus",
         512,
         342,
         1500,
         "g1.6bbscz",
         2,
         false,
         0.5,
         true,
         "ordered",
         "floyd",
         true,
         0.95f,
         false,
         {"null", "z32", "lines:count=30", "invert"}},
        {"performer",
         512,
         342,
         5000,
         "g1.6bsc",
         2,
         false,
         0.5,
         true,
         "blue",
         "floyd",
         true,
         0.95f,
         false,
         {"null", "z32", "lines:count=30", "invert"}},
        {"portable",
         640,
         400,
         2500,
         "g1.6bsc",
         2,
         false,
         0.5,
         true,
         "error",
         "floyd",
         true,
         0.98f,
         false,
         {"null", "z32", "lines:count=50", "invert"}},
        {"se",
         512,
         342,
         2500,
         "g1.6bsc",
         2,
         false,
         0.5,
         true,
         "error",
         "floyd",
         true,
         0.98f,
         false,
         {"null", "z32", "lines:count=50", "invert"}},
        {"se30",
         512,
         342,
         6000,
         "g1.6sc",
         1,
         true,
         0.3,
         false,
         "error",
         "floyd",
         true,
         0.99f,
         false,
         {"null", "z32", "lines:count=70", "invert"}},
        {"perfect",
         512,
         342,
         32000,
         "g1.6sc",
         1,
         true,
         0.3,
         false,
         "error",
         "floyd",
         true,
         1.0f,
         false,
         {"null", "z32", "lines:count=70", "invert"}},
    };

    static std::vector<std::string> parse_codec_array(const char *const *codec_array);

  public:
    static bool profile_named(const std::string name, encoding_profile &result);

    std::string dither_string() const;

    std::string description() const;
};

} // namespace macflim
