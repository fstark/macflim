#pragma once

/// Streaming protocol wire format definitions and serialization.
///
/// Designed for a 68000 Mac client: big-endian throughout (native on 68K),
/// all multi-byte fields at even offsets (68K faults on unaligned word/long),
/// fixed-size packets where possible, and minimal branching in the hot path.
///
/// Wire layout (offsets in bytes):
///
///   HELLO (client → server) — 22 bytes, fixed:
///     0: magic    "FLMS"  (uint32)
///     4: version          (uint16)
///     6: width            (uint16)
///     8: height           (uint16)
///    10: byterate         (uint16)
///    12: dither           (uint8)
///    13: num_codecs       (uint8, max 8)
///    14: codecs[8]        (uint8 × 8, zero-padded)
///
///   HELLO_ACK (server → client) — same layout, magic "FLMA"
///
///   FRAME (server → client) — 10-byte header + variable video:
///     0: magic    "FLMF"  (uint32)
///     4: seq              (uint32)
///     8: ticks            (uint16)
///    10: video data       (N bytes — codec header + delta, straight to decoder)
///
///   FEEDBACK (client → server) — 24 bytes, fixed:
///     0: magic    "FLMR"  (uint32)
///     4: last_displayed   (uint32)
///     8: history[16]      (128 bits, bit 0 of byte 0 = last_displayed)
///
/// On the 68K side, the hot path is:
///   Receive:  CMP.L #'FLMF',(A0) / MOVE.L 4(A0),D0 / MOVE.W 8(A0),D1 / LEA 10(A0),A1
///   Send:     MOVE.L #'FLMR',(A0) / MOVE.L D0,4(A0) / copy 16 bytes at 8(A0)

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace macflim
{

// --- Magic values (big-endian uint32 — one CMP.L on 68K) ---

constexpr uint32_t MAGIC_HELLO = 0x464C4D53;     // "FLMS"
constexpr uint32_t MAGIC_HELLO_ACK = 0x464C4D41; // "FLMA"
constexpr uint32_t MAGIC_FRAME = 0x464C4D46;     // "FLMF"
constexpr uint32_t MAGIC_FEEDBACK = 0x464C4D52;  // "FLMR"

constexpr uint16_t PROTOCOL_VERSION = 1;

constexpr size_t MAX_CODECS = 8;
constexpr size_t HISTORY_BYTES = 16; // 128 bits of frame history
constexpr size_t HISTORY_BITS = HISTORY_BYTES * 8;

// --- Fixed packet sizes ---

constexpr size_t HELLO_SIZE = 22;
constexpr size_t HELLO_ACK_SIZE = 22;
constexpr size_t FRAME_HEADER_SIZE = 10;
constexpr size_t FEEDBACK_SIZE = 24;

// --- Packet structs (native types, serialized to big-endian on wire) ---

/// Session setup request (client → server).
struct hello_packet
{
    uint16_t version = PROTOCOL_VERSION;
    uint16_t width = 0;
    uint16_t height = 0;
    uint16_t byterate = 0;
    uint8_t dither = 0;
    uint8_t num_codecs = 0;
    std::array<uint8_t, MAX_CODECS> codecs = {};
};

/// Session setup response (server → client). Same layout as hello, different magic.
struct hello_ack_packet
{
    uint16_t version = PROTOCOL_VERSION;
    uint16_t width = 0;
    uint16_t height = 0;
    uint16_t byterate = 0;
    uint8_t dither = 0;
    uint8_t num_codecs = 0;
    std::array<uint8_t, MAX_CODECS> codecs = {};
};

/// Frame header (server → client). Video data follows immediately.
struct frame_header
{
    uint32_t seq = 0;
    uint16_t ticks = 1;
};

/// Client feedback (client → server). Fixed 24 bytes.
struct feedback_packet
{
    uint32_t last_displayed_seq = 0;
    std::array<uint8_t, HISTORY_BYTES> history = {};
};

// --- Serialization (native structs → big-endian wire bytes) ---

[[nodiscard]] std::vector<uint8_t> serialize(const hello_packet &p);
[[nodiscard]] std::vector<uint8_t> serialize(const hello_ack_packet &p);
[[nodiscard]] std::vector<uint8_t> serialize(const frame_header &hdr, std::span<const uint8_t> video_data);
[[nodiscard]] std::vector<uint8_t> serialize(const feedback_packet &p);

// --- Deserialization (big-endian wire bytes → native structs) ---
// All return nullopt on truncated data or wrong magic.

[[nodiscard]] std::optional<hello_packet> parse_hello(const uint8_t *data, size_t len);
[[nodiscard]] std::optional<hello_ack_packet> parse_hello_ack(const uint8_t *data, size_t len);

/// Parsed frame: header fields + a view into the original buffer for video data (zero-copy).
struct frame_view
{
    frame_header header;
    const uint8_t *video_data = nullptr;
    size_t video_len = 0;
};

[[nodiscard]] std::optional<frame_view> parse_frame(const uint8_t *data, size_t len);
[[nodiscard]] std::optional<feedback_packet> parse_feedback(const uint8_t *data, size_t len);

/// Identify a packet's type from its magic bytes without fully parsing it.
/// Returns the magic value, or nullopt if fewer than 4 bytes.
[[nodiscard]] std::optional<uint32_t> peek_magic(const uint8_t *data, size_t len);

} // namespace macflim
