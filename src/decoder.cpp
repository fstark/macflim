#include "decoder.hpp"

#include "imgcompress.hpp"

#include <cassert>
#include <format>
#include <stdexcept>

namespace macflim
{

//  Decode a z32 delta: runs of uint32 values in vertical-packed bitmap space
static void decode_z32(bitmap &screen, const uint8_t *data, size_t len)
{
    const uint8_t *p = data;
    const uint8_t *end = data + len;

    size_t rowbytes = screen.W() / 8;
    size_t H = screen.H();

    //  Get the current screen as vertical uint32 values
    auto values = screen.raw_values<uint32_t>();

    while (p + 4 <= end)
    {
        //  Read 4-byte header: (count-1) << 16 | stored_offset
        uint32_t header = read4(p);
        if (header == 0)
            break; //  Terminator

        //  Decode the count
        size_t count = (header >> 16) + 1;

        //  Decode stored offset: low 14 bits of T-offset are in bits 15:2,
        //  high 2 bits of T-offset are in bits 1:0 (ror word right by 2 to reconstruct)
        uint16_t stored = header & 0xFFFF;
        size_t t_offset = ((stored & 0x3) << 14) | (stored >> 2);
        size_t byte_offset = (t_offset - 1) * sizeof(uint32_t);

        //  Convert horizontal byte offset to vertical T-index
        size_t scr_x = (byte_offset % rowbytes) / sizeof(uint32_t);
        size_t scr_y = byte_offset / rowbytes;
        size_t vert_idx = scr_x * H + scr_y;

        //  Write count values downward in the vertical column
        for (size_t i = 0; i < count && p + 4 <= end; i++)
        {
            assert(vert_idx + i < values.size());
            values[vert_idx + i] = read4(p);
        }
    }

    //  Reconstruct bitmap from modified vertical data
    screen = bitmap{values, screen.W(), screen.H()};
}

//  Decode a z16 delta: runs of uint16 values with relative offsets
static void decode_z16(bitmap &screen, const uint8_t *data, size_t len)
{
    const uint8_t *p = data;
    const uint8_t *end = data + len;

    size_t rowbytes = screen.W() / 8;
    size_t H = screen.H();

    //  Get the current screen as vertical uint16 values
    auto values = screen.raw_values<uint16_t>();

    //  Current position tracks the running horizontal T-index
    size_t current_pos = 0;

    while (p + 2 <= end)
    {
        //  Read 2-byte header: skip << 8 | count
        uint16_t header = read2(p);
        if (header == 0)
            break; //  Terminator

        size_t skip = header >> 8;
        size_t count = header & 0xFF;

        //  Advance by skip in horizontal T-index space
        current_pos += skip;

        //  Convert horizontal T-index to vertical T-index
        size_t byte_offset = current_pos * sizeof(uint16_t);
        size_t scr_x = (byte_offset % rowbytes) / sizeof(uint16_t);
        size_t scr_y = byte_offset / rowbytes;
        size_t vert_idx = scr_x * H + scr_y;

        //  Write count values downward in the vertical column
        for (size_t i = 0; i < count && p + 2 <= end; i++)
        {
            assert(vert_idx + i < values.size());
            values[vert_idx + i] = read2(p);
        }
    }

    //  Reconstruct bitmap from modified vertical data
    screen = bitmap{values, screen.W(), screen.H()};
}

//  Decode a lines delta: copy a block of scanlines
static void decode_lines(bitmap &screen, const uint8_t *data, size_t len)
{
    if (len < 4)
        return; //  Not enough data for header

    const uint8_t *p = data;

    //  Read byte_count and byte_offset
    uint16_t byte_count = read2(p);
    uint16_t byte_offset = read2(p);

    if (byte_count == 0)
        return; //  Nothing to copy

    //  The data is raw horizontal scanline bytes, copied at byte_offset
    auto raw = screen.raw_data();
    assert(byte_offset + byte_count <= raw.size());
    assert(p + byte_count <= data + len);

    std::copy(p, p + byte_count, raw.begin() + byte_offset);

    //  Reconstruct bitmap from modified raw data
    screen = bitmap{raw, screen.W(), screen.H(), false};
}

//  Decode an invert delta: flip all pixels
static void decode_invert(bitmap &screen)
{
    auto raw = screen.raw_data();
    for (auto &byte : raw)
        byte ^= 0xFF;
    screen = bitmap{raw, screen.W(), screen.H(), false};
}

void apply_delta(bitmap &screen, uint8_t codec_signature, const uint8_t *data, size_t len)
{
    switch (codec_signature)
    {
    case 0x00: //  null — no change
        return;
    case 0x01: //  z16
        decode_z16(screen, data, len);
        return;
    case 0x02: //  z32
        decode_z32(screen, data, len);
        return;
    case 0x03: //  invert
        decode_invert(screen);
        return;
    case 0x04: //  lines
        decode_lines(screen, data, len);
        return;
    default:
        throw std::runtime_error(std::format("Unknown codec signature: 0x{:02x}", codec_signature));
    }
}

void apply_delta(bitmap &screen, const std::vector<uint8_t> &encoded_data)
{
    if (encoded_data.size() < 4)
        throw std::runtime_error("Encoded data too short for codec header");

    //  Extract codec signature from 4-byte header (0x00 0x00 0x00 <sig>)
    uint8_t signature = encoded_data[3];
    const uint8_t *payload = encoded_data.data() + 4;
    size_t payload_len = encoded_data.size() - 4;

    apply_delta(screen, signature, payload, payload_len);
}

} // namespace macflim
