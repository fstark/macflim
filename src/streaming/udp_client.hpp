#pragma once

/// Client-side UDP socket for the streaming protocol.
/// Connects to a macflim streaming server, sends HELLO/FEEDBACK,
/// receives HELLO_ACK/FRAME. Counterpart to the server-side udp_transport.

#include "protocol.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>

namespace macflim
{

/// Client-side UDP socket for streaming from a macflim server.
class udp_client
{
  public:
    udp_client(std::string_view server_host, uint16_t server_port);
    ~udp_client();

    udp_client(const udp_client &) = delete;
    udp_client &operator=(const udp_client &) = delete;

    /// Send a HELLO packet to the server.
    void send_hello(const hello_packet &hello);

    /// Block until a HELLO_ACK arrives. Throws on timeout or wrong packet.
    [[nodiscard]] hello_ack_packet wait_for_hello_ack();

    /// Non-blocking receive of a FRAME packet. Returns nullopt if nothing available.
    /// The returned vector is the raw packet bytes (caller parses with parse_frame).
    [[nodiscard]] std::optional<std::vector<uint8_t>> receive_packet();

    /// Send a FEEDBACK packet to the server.
    void send_feedback(const feedback_packet &fb);

  private:
    struct impl;
    std::unique_ptr<impl> impl_;
};

} // namespace macflim
