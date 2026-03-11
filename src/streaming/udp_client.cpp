#include "udp_client.hpp"

#include "protocol.hpp"

#include <array>
#include <cstring>
#include <format>
#include <stdexcept>
#include <vector>

// --- Platform abstraction (mirrors udp_transport.cpp) ---

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

struct udp_client::impl
{
    socket_t sock = INVALID_SOCK;
    sockaddr_in server_addr = {};

    static constexpr size_t MAX_PACKET = 65536;

    impl(std::string_view server_host, uint16_t server_port)
    {
        platform_init();

        sock = ::socket(AF_INET, SOCK_DGRAM, 0);
        if (sock == INVALID_SOCK)
            throw std::runtime_error("socket() failed");

        server_addr = {};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(server_port);
        if (inet_pton(AF_INET, std::string(server_host).c_str(), &server_addr.sin_addr) != 1)
            throw std::runtime_error(std::format("Invalid server address: {}", server_host));
    }

    ~impl()
    {
        if (sock != INVALID_SOCK)
            platform_close(sock);
    }

    void send_to_server(const std::vector<uint8_t> &data)
    {
        ::sendto(sock, reinterpret_cast<const char *>(data.data()), data.size(), 0,
                 reinterpret_cast<sockaddr *>(&server_addr), sizeof(server_addr));
    }

    //  Non-blocking receive. Returns bytes read, or 0 if nothing available.
    //  Throws if the datagram was truncated (buffer too small).
    ssize_t recv(uint8_t *buf, size_t buf_size)
    {
        struct iovec iov = {};
        iov.iov_base = buf;
        iov.iov_len = buf_size;

        struct msghdr msg = {};
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;

        auto n = ::recvmsg(sock, &msg, 0);
        if (n < 0)
        {
            if (platform_would_block(platform_last_error()))
                return 0;
            throw std::runtime_error("recvmsg() failed");
        }

        if (msg.msg_flags & MSG_TRUNC)
            throw std::runtime_error(std::format(
                "UDP packet truncated: buffer is {} bytes but datagram was larger (received {})", buf_size, n));

        return n;
    }
};

// --- Construction ---

udp_client::udp_client(std::string_view server_host, uint16_t server_port)
    : impl_{std::make_unique<impl>(server_host, server_port)}
{
}

udp_client::~udp_client() = default;

// --- Protocol operations ---

void udp_client::send_hello(const hello_packet &hello)
{
    impl_->send_to_server(serialize(hello));
}

hello_ack_packet udp_client::wait_for_hello_ack()
{
    //  Socket starts in blocking mode — wait for the ACK
    std::array<uint8_t, impl::MAX_PACKET> buf;
    auto n = impl_->recv(buf.data(), buf.size());
    if (n <= 0)
        throw std::runtime_error("No response from server");

    auto ack = parse_hello_ack(buf.data(), static_cast<size_t>(n));
    if (!ack)
        throw std::runtime_error("Expected HELLO_ACK, got something else");

    //  Switch to non-blocking for the streaming loop
    platform_set_nonblocking(impl_->sock);

    return *ack;
}

std::optional<std::vector<uint8_t>> udp_client::receive_packet()
{
    std::array<uint8_t, impl::MAX_PACKET> buf;
    auto n = impl_->recv(buf.data(), buf.size());
    if (n <= 0)
        return std::nullopt;

    return std::vector<uint8_t>(buf.begin(), buf.begin() + n);
}

void udp_client::send_feedback(const feedback_packet &fb)
{
    impl_->send_to_server(serialize(fb));
}

} // namespace macflim
