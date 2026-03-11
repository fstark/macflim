#include "udp_transport.hpp"

#include "protocol.hpp"

#include <array>
#include <cstring>
#include <format>
#include <stdexcept>
#include <vector>

// --- Platform abstraction ---

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

namespace
{
using socket_t = SOCKET;
constexpr socket_t INVALID_SOCK = INVALID_SOCKET;

void platform_init()
{
    static bool done = false;
    if (!done)
    {
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
            throw std::runtime_error("WSAStartup failed");
        done = true;
    }
}

void platform_close(socket_t s)
{
    closesocket(s);
}

void platform_set_nonblocking(socket_t s)
{
    u_long mode = 1;
    if (ioctlsocket(s, FIONBIO, &mode) != 0)
        throw std::runtime_error(std::format("ioctlsocket failed: {}", WSAGetLastError()));
}

int platform_last_error()
{
    return WSAGetLastError();
}

bool platform_would_block(int err)
{
    return err == WSAEWOULDBLOCK;
}

} // namespace

#else
#include <arpa/inet.h>
#include <cerrno>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace
{
using socket_t = int;
constexpr socket_t INVALID_SOCK = -1;

void platform_init()
{
    // No-op on POSIX
}

void platform_close(socket_t s)
{
    ::close(s);
}

void platform_set_nonblocking(socket_t s)
{
    int flags = fcntl(s, F_GETFL, 0);
    if (flags < 0 || fcntl(s, F_SETFL, flags | O_NONBLOCK) < 0)
        throw std::runtime_error(std::format("fcntl failed: {}", strerror(errno)));
}

int platform_last_error()
{
    return errno;
}

bool platform_would_block(int err)
{
    return err == EAGAIN || err == EWOULDBLOCK;
}

} // namespace
#endif

// --- Implementation ---

namespace macflim
{

struct udp_transport::impl
{
    socket_t sock = INVALID_SOCK;
    sockaddr_in client_addr = {};
    bool has_client = false;

    static constexpr size_t MAX_PACKET = 65536;

    impl(uint16_t local_port)
    {
        platform_init();
        sock = ::socket(AF_INET, SOCK_DGRAM, 0);
        if (sock == INVALID_SOCK)
            throw std::runtime_error("socket() failed");

        //  Allow address reuse for quick restart
        int reuse = 1;
        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char *>(&reuse), sizeof(reuse));

        sockaddr_in local_addr = {};
        local_addr.sin_family = AF_INET;
        local_addr.sin_addr.s_addr = INADDR_ANY;
        local_addr.sin_port = htons(local_port);

        if (::bind(sock, reinterpret_cast<sockaddr *>(&local_addr), sizeof(local_addr)) < 0)
            throw std::runtime_error(std::format("bind() to port {} failed", local_port));

        platform_set_nonblocking(sock);
    }

    ~impl()
    {
        if (sock != INVALID_SOCK)
            platform_close(sock);
    }

    void set_client(std::string_view host, uint16_t port)
    {
        client_addr = {};
        client_addr.sin_family = AF_INET;
        client_addr.sin_port = htons(port);
        if (inet_pton(AF_INET, std::string(host).c_str(), &client_addr.sin_addr) != 1)
            throw std::runtime_error(std::format("Invalid client address: {}", host));
        has_client = true;
    }

    void send_to_client(const std::vector<uint8_t> &data)
    {
        if (!has_client)
            throw std::runtime_error("No client address set");
        ::sendto(sock, reinterpret_cast<const char *>(data.data()), data.size(), 0,
                 reinterpret_cast<sockaddr *>(&client_addr), sizeof(client_addr));
    }

    //  Non-blocking receive. Returns the number of bytes read, or 0 if nothing available.
    //  Stores the sender address in from_addr.
    ssize_t recv_from(uint8_t *buf, size_t buf_size, sockaddr_in &from_addr)
    {
        socklen_t addr_len = sizeof(from_addr);
        auto n = ::recvfrom(sock, reinterpret_cast<char *>(buf), buf_size, 0, reinterpret_cast<sockaddr *>(&from_addr),
                            &addr_len);
        if (n < 0)
        {
            if (platform_would_block(platform_last_error()))
                return 0;
            throw std::runtime_error("recvfrom() failed");
        }
        return n;
    }
};

// --- Constructors ---

udp_transport::udp_transport(uint16_t local_port) : impl_{std::make_unique<impl>(local_port)} {}

udp_transport::udp_transport(uint16_t local_port, std::string_view client_host, uint16_t client_port)
    : impl_{std::make_unique<impl>(local_port)}
{
    impl_->set_client(client_host, client_port);
}

udp_transport::~udp_transport() = default;

// --- transport interface ---

void udp_transport::send_hello_ack(const hello_ack_packet &ack)
{
    impl_->send_to_client(serialize(ack));
}

void udp_transport::send_frame(const frame_header &hdr, std::span<const uint8_t> video_data)
{
    impl_->send_to_client(serialize(hdr, video_data));
}

std::optional<feedback_packet> udp_transport::receive_feedback()
{
    std::array<uint8_t, impl::MAX_PACKET> buf;
    sockaddr_in from_addr;
    auto n = impl_->recv_from(buf.data(), buf.size(), from_addr);
    if (n <= 0)
        return std::nullopt;

    //  Try to parse as feedback
    auto fb = parse_feedback(buf.data(), static_cast<size_t>(n));
    if (fb)
        return fb;

    //  Ignore unrecognized packets (could be a stale HELLO, etc.)
    return std::nullopt;
}

// --- Server-side handshake ---

hello_packet udp_transport::wait_for_hello()
{
    //  Temporarily set blocking mode for the handshake wait
    //  (we'll go back to non-blocking after)
#ifdef _WIN32
    u_long mode = 0;
    ioctlsocket(impl_->sock, FIONBIO, &mode);
#else
    int flags = fcntl(impl_->sock, F_GETFL, 0);
    fcntl(impl_->sock, F_SETFL, flags & ~O_NONBLOCK);
#endif

    std::array<uint8_t, impl::MAX_PACKET> buf;
    sockaddr_in from_addr;
    socklen_t addr_len = sizeof(from_addr);

    ssize_t n = ::recvfrom(impl_->sock, reinterpret_cast<char *>(buf.data()), buf.size(), 0,
                           reinterpret_cast<sockaddr *>(&from_addr), &addr_len);
    if (n < 0)
        throw std::runtime_error("recvfrom() failed waiting for HELLO");

    //  Restore non-blocking
    platform_set_nonblocking(impl_->sock);

    auto hello = parse_hello(buf.data(), static_cast<size_t>(n));
    if (!hello)
        throw std::runtime_error("Expected HELLO packet, got something else");

    //  Remember the client address
    impl_->client_addr = from_addr;
    impl_->has_client = true;

    return *hello;
}

bool udp_transport::has_client() const
{
    return impl_->has_client;
}

} // namespace macflim
