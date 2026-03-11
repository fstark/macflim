#pragma once

/// Abstract frame source: yields the next target bitmap at 60Hz.
/// For option 2 (streaming .flim), the source replays pre-encoded frames.
/// For option 1 (future live encoding), the source reads video and dithers on the fly.

#include "../bitmap.hpp"

#include <functional>
#include <optional>

namespace macflim
{

/// Callable that returns the next target bitmap, or nullopt when the source is exhausted.
using frame_source = std::function<std::optional<bitmap>()>;

} // namespace macflim
