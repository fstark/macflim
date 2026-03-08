#pragma once

#include <cstdint>
#include <cstddef>

namespace macflim
{

/// Macintosh screen and image constants
namespace constants
{
    /// Classic Macintosh screen dimensions
    constexpr size_t mac_screen_width = 512;
    constexpr size_t mac_screen_height = 342;
    
    /// Pixel value ranges
    constexpr uint8_t pixel_min = 0;
    constexpr uint8_t pixel_max = 255;
    constexpr uint8_t pixel_max_threshold = 0xfc;  // Values >= this become 0xff
    constexpr uint8_t pixel_min_threshold = 0x01;  // Values <= this become 0x00
    
    /// Frame timing
    constexpr double ticks_per_second = 60.0;
    
    /// Sound frame size (Mac format)
    constexpr size_t sound_frame_bytes = 370;
}

} // namespace macflim
