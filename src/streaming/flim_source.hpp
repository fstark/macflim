#pragma once

/// Builds a frame_source from a .flim file by replaying all frame deltas.
/// The .flim is fully loaded and decoded into a vector of target bitmaps upfront,
/// then the source yields them one at a time.

#include "frame_source.hpp"

#include <string_view>

namespace macflim
{

/// Load a .flim file and return a frame_source that yields its decoded target bitmaps.
/// The entire .flim is decoded into memory on construction.
[[nodiscard]] frame_source make_flim_source(std::string_view path);

} // namespace macflim
