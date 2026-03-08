#pragma once

#include "common.hpp"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace macflim
{

//  ------------------------------------------------------------------
//  A grayscale class and various associated utilities
//  ------------------------------------------------------------------

/// Grayscale image represented as floating point values (0=black, 1=white).
/// Used as the intermediate format for dithering and image processing operations.
class grayscale
{
  private:
    std::vector<float> grayscale_;
    size_t W_;
    size_t H_;

  public:
    grayscale(size_t W, size_t H) : grayscale_(W * H), W_{W}, H_{H} {}

    // Explicit move semantics for large data
    grayscale(grayscale &&) noexcept = default;
    grayscale &operator=(grayscale &&) noexcept = default;

    // Default copy operations
    grayscale(const grayscale &) = default;
    grayscale &operator=(const grayscale &) = default;

    [[nodiscard]] size_t W() const
    {
        return W_;
    }
    [[nodiscard]] size_t H() const
    {
        return H_;
    }

    const float &at(size_t x, size_t y) const
    {
        assert(x < W_);
        assert(y < H_);
        return grayscale_[x + y * W_];
    }
    float &at(size_t x, size_t y)
    {
        assert(x < W_);
        assert(y < H_);
        return grayscale_[x + y * W_];
    }

    enum class dithering
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
[[nodiscard]] grayscale round_corners(const grayscale &img);
[[nodiscard]] grayscale filter(const grayscale &from, const char *filters);
void ordered_dither(grayscale &dest, const grayscale &source, const grayscale &previous);
void blue_noise_dither(grayscale &dest, const grayscale &source, const grayscale &previous);

struct dither_algorithm;

[[nodiscard]] const dither_algorithm *get_error_diffusion_by_name(std::string_view name);
void error_diffusion_algorithms(std::function<void(std::string_view name, std::string_view description)> f);
void error_diffusion(grayscale &dest, const grayscale &source, const grayscale &previous, float stability,
                     const dither_algorithm &algo, float bleed = 1, bool two_ways = false);

[[nodiscard]] bool read_grayscale(grayscale &result, std::string_view file);
void write_grayscale(std::string_view file, const grayscale &img);

void copy(grayscale &destination, const grayscale &source, bool black_bars = true, double anchor_x = 0.5,
          double anchor_y = 0.5);

} // namespace macflim
