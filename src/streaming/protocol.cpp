#include "protocol.hpp"

#include "../imgcompress.hpp"

#include <algorithm>

namespace macflim
{

// --- Big-endian write helpers (append to vector) ---

namespace
{

void push_u32(std::vector<uint8_t> &buf, uint32_t v)
{
    buf.push_back(static_cast<uint8_t>(v >> 24));
    buf.push_back(static_cast<uint8_t>(v >> 16));
    buf.push_back(static_cast<uint8_t>(v >> 8));
    buf.push_back(static_cast<uint8_t>(v));
}

void push_u16(std::vector<uint8_t> &buf, uint16_t v)
{
    buf.push_back(static_cast<uint8_t>(v >> 8));
    buf.push_back(static_cast<uint8_t>(v));
}

/// Serialize the common hello/hello_ack body (everything after magic).
void push_hello_body(std::vector<uint8_t> &buf, uint16_t version, uint16_t width, uint16_t height, uint16_t byterate,
                     uint8_t dither, uint8_t num_codecs, const std::array<uint8_t, MAX_CODECS> &codecs)
{
    push_u16(buf, version);
    push_u16(buf, width);
    push_u16(buf, height);
    push_u16(buf, byterate);
    buf.push_back(dither);
    buf.push_back(num_codecs);
    buf.insert(buf.end(), codecs.begin(), codecs.end());
}

/// Parse the common hello/hello_ack body from wire data at offset 4 (after magic).
/// Returns false if len < HELLO_SIZE.
template <typename T> bool parse_hello_body(T &out, const uint8_t *data, [[maybe_unused]] size_t len)
{
    const uint8_t *p = data + 4; // skip magic
    out.version = read2(p);
    out.width = read2(p);
    out.height = read2(p);
    out.byterate = read2(p);
    out.dither = *p++;
    out.num_codecs = *p++;
    std::copy_n(p, MAX_CODECS, out.codecs.begin());
    return true;
}

} // namespace

// --- Serialization ---

std::vector<uint8_t> serialize(const hello_packet &p)
{
    std::vector<uint8_t> buf;
    buf.reserve(HELLO_SIZE);
    push_u32(buf, MAGIC_HELLO);
    push_hello_body(buf, p.version, p.width, p.height, p.byterate, p.dither, p.num_codecs, p.codecs);
    return buf;
}

std::vector<uint8_t> serialize(const hello_ack_packet &p)
{
    std::vector<uint8_t> buf;
    buf.reserve(HELLO_ACK_SIZE);
    push_u32(buf, MAGIC_HELLO_ACK);
    push_hello_body(buf, p.version, p.width, p.height, p.byterate, p.dither, p.num_codecs, p.codecs);
    return buf;
}

std::vector<uint8_t> serialize(const frame_header &hdr, std::span<const uint8_t> video_data)
{
    std::vector<uint8_t> buf;
    buf.reserve(FRAME_HEADER_SIZE + video_data.size());
    push_u32(buf, MAGIC_FRAME);
    push_u32(buf, hdr.seq);
    push_u16(buf, hdr.ticks);
    buf.insert(buf.end(), video_data.begin(), video_data.end());
    return buf;
}

std::vector<uint8_t> serialize(const feedback_packet &p)
{
    std::vector<uint8_t> buf;
    buf.reserve(FEEDBACK_SIZE);
    push_u32(buf, MAGIC_FEEDBACK);
    push_u32(buf, p.last_displayed_seq);
    buf.insert(buf.end(), p.history.begin(), p.history.end());
    return buf;
}

// --- Deserialization ---

std::optional<uint32_t> peek_magic(const uint8_t *data, size_t len)
{
    if (len < 4)
        return std::nullopt;
    const uint8_t *p = data;
    return read4(p);
}

std::optional<hello_packet> parse_hello(const uint8_t *data, size_t len)
{
    if (len < HELLO_SIZE)
        return std::nullopt;

    const uint8_t *p = data;
    if (read4(p) != MAGIC_HELLO)
        return std::nullopt;

    hello_packet pkt;
    parse_hello_body(pkt, data, len);
    return pkt;
}

std::optional<hello_ack_packet> parse_hello_ack(const uint8_t *data, size_t len)
{
    if (len < HELLO_ACK_SIZE)
        return std::nullopt;

    const uint8_t *p = data;
    if (read4(p) != MAGIC_HELLO_ACK)
        return std::nullopt;

    hello_ack_packet pkt;
    parse_hello_body(pkt, data, len);
    return pkt;
}

std::optional<frame_view> parse_frame(const uint8_t *data, size_t len)
{
    if (len < FRAME_HEADER_SIZE)
        return std::nullopt;

    const uint8_t *p = data;
    if (read4(p) != MAGIC_FRAME)
        return std::nullopt;

    frame_view fv;
    fv.header.seq = read4(p);
    fv.header.ticks = read2(p);
    fv.video_data = data + FRAME_HEADER_SIZE;
    fv.video_len = len - FRAME_HEADER_SIZE;
    return fv;
}

std::optional<feedback_packet> parse_feedback(const uint8_t *data, size_t len)
{
    if (len < FEEDBACK_SIZE)
        return std::nullopt;

    const uint8_t *p = data;
    if (read4(p) != MAGIC_FEEDBACK)
        return std::nullopt;

    feedback_packet pkt;
    pkt.last_displayed_seq = read4(p);
    std::copy_n(p, HISTORY_BYTES, pkt.history.begin());
    return pkt;
}

} // namespace macflim
