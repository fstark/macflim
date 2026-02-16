#pragma once

#include <cstdint>

#include <cassert>
#include <string>
#include <vector>

#include <functional>

#include "common.hpp"
#include <algorithm>

namespace macflim
{

//  ------------------------------------------------------------------
//  A grayscale class and various associated utilities
//  ------------------------------------------------------------------

//  This is a grayscale image, represented as a bunch of floating point values (0==black and 1==white)
//  Sometime, a pixel can be <0 or >1, when error propagates during dithering
class grayscale
{
  public:
    std::vector<float> grayscale_;
    size_t W_;
    size_t H_;

  public:
    grayscale(size_t W, size_t H) : grayscale_(W * H), W_{W}, H_{H} {}

    size_t W() const
    {
        return W_;
    }
    size_t H() const
    {
        return H_;
    }

    const float &at(size_t x, size_t y) const
    {
        assert(x < W_);
        assert(y < H_);
        assert(x < W_);
        assert(y < H_);
        return grayscale_[x + y * W_];
    }
    float &at(size_t x, size_t y)
    {
        assert(x < W_);
        assert(y < H_);
        assert(x < W_);
        assert(y < H_);
        return grayscale_[x + y * W_];
    }

    enum dithering
    {
        error_diffusion = 0,
        ordered = 1,
        blue_noise = 2
    };

    //  Dealing with ffmpeg data

    void set_luma(const uint8_t *y) //  Sets the (monochrome) content from a Y luma buffer of W*H bytes
    {
        std::transform(y, y + W_ * H_, std::begin(grayscale_), [](auto v) { return v / 255.0; });
    }
};

void fill(grayscale &img, float value = 0.5);
grayscale round_corners(const grayscale &img);
grayscale filter(const grayscale &from, const char *filters);
void ordered_dither(grayscale &dest, const grayscale &source, const grayscale &previous);
void blue_noise_dither(grayscale &dest, const grayscale &source, const grayscale &previous);

struct dither_algorithm;

const dither_algorithm *get_error_diffusion_by_name(const std::string &name);
void error_diffusion_algorithms(std::function<void(const std::string name, const std::string desciption)> f);
void error_diffusion(grayscale &dest, const grayscale &source, const grayscale &previous, float stability,
                     const dither_algorithm &algo, float bleed = 1, bool two_ways = false);

bool read_grayscale(grayscale &result, const char *file);
void write_grayscale(const char *file, const grayscale &img);

void copy(grayscale &destination, const grayscale &source, bool black_bars = true, double anchor_x = 0.5,
          double anchor_y = 0.5);

} // namespace macflim
