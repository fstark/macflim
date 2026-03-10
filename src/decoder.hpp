#pragma once

#include "bitmap.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace macflim
{

/// Apply an encoded video delta to a screen bitmap in-place.
/// The encoded_data must begin with the 4-byte codec header (0x00 0x00 0x00 <signature>)
/// followed by the codec-specific delta payload.
/// Used by server-side screen simulation and the Linux test player.
void apply_delta(bitmap &screen, const std::vector<uint8_t> &encoded_data);

/// Apply an encoded video delta with the codec signature provided separately.
/// data points to the codec-specific payload (no header).
void apply_delta(bitmap &screen, uint8_t codec_signature,
                 const uint8_t *data, size_t len);

} // namespace macflim
