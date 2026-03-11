#pragma once

/// UDP transport implementation for the streaming protocol.
/// Wraps POSIX/Winsock2 sockets behind the transport interface, sending FRAME and
/// HELLO_ACK packets to the client and receiving FEEDBACK packets non-blockingly.
///
/// The server creates this after receiving a HELLO from a client. The client address
/// is stored at construction time (from the source of the HELLO packet).

#include "transport.hpp"

#include <cstdint>
#include <memory>
#include <string_view>

namespace macflim
{

/// Server-side UDP transport: sends frames to a known client, receives feedback.
class udp_transport final : public transport
{
  public:
    /// Construct a server transport bound to a local port.
    /// The client address is set when the first HELLO is received via wait_for_hello().
    explicit udp_transport(uint16_t local_port);

    /// Construct a server transport with a known client address (e.g. from a prior HELLO).
    udp_transport(uint16_t local_port, std::string_view client_host, uint16_t client_port);

    ~udp_transport() override;

    udp_transport(const udp_transport &) = delete;
    udp_transport &operator=(const udp_transport &) = delete;

    // --- transport interface ---

    void send_hello_ack(const hello_ack_packet &ack) override;
    void send_frame(const frame_header &hdr, std::span<const uint8_t> video_data) override;
    [[nodiscard]] std::optional<feedback_packet> receive_feedback() override;

    // --- Server-side handshake ---

    /// Block until a HELLO packet arrives. Returns the parsed packet and stores the client address.
    [[nodiscard]] hello_packet wait_for_hello();

    /// True if a client address has been set (either via constructor or wait_for_hello).
    [[nodiscard]] bool has_client() const;

  private:
    struct impl;
    std::unique_ptr<impl> impl_;
};

} // namespace macflim
