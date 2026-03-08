#include "imgcompress.hpp"

namespace macflim
{

//  ------------------------------------------------------------------
//  Almost completely tested implementation of packbits
//  Compresses 'length' bytes from 'buffer' into 'out', and return the compressed size
//  (unused for now)
//  ------------------------------------------------------------------
//  Find the next pair of identical adjacent bytes, or return end if none
static const uint8_t *find_next_run(const uint8_t *buffer, const uint8_t *end)
{
    for (const uint8_t *p = buffer; p < end - 1; p++)
        if (p[0] == p[1])
            return p;
    return end;
}

//  Emit literal (non-repeating) bytes, up to 128 at a time
static void emit_literals(uint8_t *&out, const uint8_t *&buffer, const uint8_t *literal_end)
{
    uint32_t len = literal_end - buffer;
    while (len)
    {
        uint8_t sub_length = len > 128 ? 128 : len;
        len -= sub_length;
        *out++ = sub_length - 1;
        while (sub_length--)
            *out++ = *buffer++;
    }
}

//  Emit a run of identical bytes, up to 128
static void emit_run(uint8_t *&out, const uint8_t *&buffer, const uint8_t *end)
{
    uint8_t c = *buffer;
    int len = 0;
    while (buffer < end && *buffer == c && len < 128)
    {
        len++;
        buffer++;
    }
    *out++ = -len + 1;
    *out++ = c;
}

int packbits(uint8_t *out, const uint8_t *buffer, int length)
{
    const uint8_t *orig = out;
    const uint8_t *end = buffer + length;

    while (buffer < end)
    {
        const uint8_t *next_pair = find_next_run(buffer, end);

        if (next_pair != buffer)
            emit_literals(out, buffer, next_pair);

        assert(buffer == next_pair);
        if (buffer == end)
            break;

        assert(buffer < end - 1);
        emit_run(out, buffer, end);
    }

    return out - orig;
}

} // namespace macflim
