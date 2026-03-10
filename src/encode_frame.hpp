#pragma once

#include "bitmap.hpp"
#include "codec_spec.hpp"
#include "encoding_result.hpp"

#include <cstddef>
#include <vector>

namespace macflim
{

/// Encode a single frame transition: tries all codecs within budget and returns the best result.
/// After this call, current_fb is updated to reflect what was actually drawn (which may be
/// partial if budget was tight). This is the core encode step, usable by both the batch
/// pipeline and the streaming encoder.
[[nodiscard]] encoding_result encode_frame(bitmap &current_fb, const bitmap &target,
                                           const std::vector<codec_spec> &codecs, size_t budget);

} // namespace macflim
