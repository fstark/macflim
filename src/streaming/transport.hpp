#pragma once

/// Abstract transport interface for streaming protocol packets.
/// Separates the session logic from the actual network I/O, enabling
/// in-process testing with a loopback transport and future UDP/serial implementations.

#include "protocol.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace macflim
{

/// Abstract bidirectional transport for the streaming protocol.
class transport
{
  public:
    virtual ~transport() = default;

    /// Send a HELLO_ACK packet to the client.
    virtual void send_hello_ack(const hello_ack_packet &ack) = 0;

    /// Send a FRAME packet to the client.
    virtual void send_frame(const frame_header &hdr, std::span<const uint8_t> video_data) = 0;

    /// Non-blocking receive of a feedback packet. Returns nullopt if nothing available.
    [[nodiscard]] virtual std::optional<feedback_packet> receive_feedback() = 0;
};

} // namespace macflim
