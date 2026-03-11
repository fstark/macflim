#include "../doctest.h"

#include "../streaming/protocol.hpp"
#include "../streaming/udp_transport.hpp"

#include <array>
#include <chrono>
#include <cstring>
#include <thread>

using namespace macflim;

// Helper: pick a port unlikely to conflict (high ephemeral range)
static uint16_t test_port(int offset)
{
    return static_cast<uint16_t>(49200 + offset);
}

// ---------------------------------------------------------------------------
// Platform socket helpers for client-side operations in tests
// ---------------------------------------------------------------------------

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
using socket_t = SOCKET;
static void close_sock(socket_t s)
{
    closesocket(s);
}
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
using socket_t = int;
static void close_sock(socket_t s)
{
    ::close(s);
}
#endif

namespace
{

/// Minimal "client" — a raw UDP socket that sends HELLO and receives FRAME/HELLO_ACK.
struct test_client
{
    socket_t sock;
    sockaddr_in server_addr;

    test_client(uint16_t server_port)
    {
        sock = ::socket(AF_INET, SOCK_DGRAM, 0);
        server_addr = {};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(server_port);
        inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);
    }

    ~test_client()
    {
        close_sock(sock);
    }

    void send_bytes(const std::vector<uint8_t> &data)
    {
        ::sendto(sock, reinterpret_cast<const char *>(data.data()), data.size(), 0,
                 reinterpret_cast<sockaddr *>(&server_addr), sizeof(server_addr));
    }

    std::vector<uint8_t> recv_bytes(size_t timeout_ms = 500)
    {
        //  Set receive timeout
        struct timeval tv;
        tv.tv_sec = static_cast<long>(timeout_ms / 1000);
        tv.tv_usec = static_cast<long>((timeout_ms % 1000) * 1000);
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char *>(&tv), sizeof(tv));

        std::array<uint8_t, 2048> buf;
        auto n = ::recv(sock, reinterpret_cast<char *>(buf.data()), buf.size(), 0);
        if (n <= 0)
            return {};
        return {buf.begin(), buf.begin() + n};
    }
};

} // namespace

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

TEST_CASE("udp_transport — construct and has_client")
{
    udp_transport tp(test_port(0));
    CHECK_FALSE(tp.has_client());
}

TEST_CASE("udp_transport — construct with client address")
{
    udp_transport tp(test_port(1), "127.0.0.1", 12345);
    CHECK(tp.has_client());
}

TEST_CASE("udp_transport — send frame to client")
{
    uint16_t port = test_port(2);
    udp_transport tp(port, "127.0.0.1", port + 100);

    //  Create a client socket bound to port+100 to receive the frame
    socket_t client_sock = ::socket(AF_INET, SOCK_DGRAM, 0);
    sockaddr_in client_addr = {};
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(port + 100);
    client_addr.sin_addr.s_addr = INADDR_ANY;
    ::bind(client_sock, reinterpret_cast<sockaddr *>(&client_addr), sizeof(client_addr));

    //  Set receive timeout
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(client_sock, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char *>(&tv), sizeof(tv));

    //  Send a frame
    frame_header hdr;
    hdr.seq = 42;
    hdr.ticks = 1;
    std::vector<uint8_t> video = {0x00, 0x00, 0x00, 0x01, 0xAA, 0xBB};
    tp.send_frame(hdr, video);

    //  Receive and parse
    std::array<uint8_t, 2048> buf;
    auto n = ::recv(client_sock, reinterpret_cast<char *>(buf.data()), buf.size(), 0);
    REQUIRE(n > 0);

    auto parsed = parse_frame(buf.data(), static_cast<size_t>(n));
    REQUIRE(parsed.has_value());
    CHECK(parsed->header.seq == 42);
    CHECK(parsed->header.ticks == 1);
    CHECK(parsed->video_len == 6);

    close_sock(client_sock);
}

TEST_CASE("udp_transport — receive feedback from client")
{
    uint16_t port = test_port(3);
    udp_transport tp(port, "127.0.0.1", port + 100);

    //  Verify no feedback initially
    CHECK_FALSE(tp.receive_feedback().has_value());

    //  Send a feedback packet from a "client" to the server port
    test_client client(port);

    feedback_packet fb;
    fb.last_displayed_seq = 99;
    fb.history.fill(0xFF);
    client.send_bytes(serialize(fb));

    //  Small delay for the packet to arrive
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    //  Receive it
    auto received = tp.receive_feedback();
    REQUIRE(received.has_value());
    CHECK(received->last_displayed_seq == 99);
    CHECK(received->history[0] == 0xFF);
}

TEST_CASE("udp_transport — send hello_ack")
{
    uint16_t port = test_port(4);
    test_client client(port);

    //  Bind client to a known port so the server can send back
    socket_t client_sock = ::socket(AF_INET, SOCK_DGRAM, 0);
    sockaddr_in client_addr = {};
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(port + 100);
    client_addr.sin_addr.s_addr = INADDR_ANY;
    ::bind(client_sock, reinterpret_cast<sockaddr *>(&client_addr), sizeof(client_addr));

    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(client_sock, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char *>(&tv), sizeof(tv));

    //  Create transport with known client
    udp_transport tp(port, "127.0.0.1", port + 100);

    hello_ack_packet ack;
    ack.width = 512;
    ack.height = 342;
    ack.byterate = 6000;
    ack.num_codecs = 2;
    ack.codecs[0] = 0x01;
    ack.codecs[1] = 0x02;
    tp.send_hello_ack(ack);

    //  Receive and parse
    std::array<uint8_t, 2048> buf;
    auto n = ::recv(client_sock, reinterpret_cast<char *>(buf.data()), buf.size(), 0);
    REQUIRE(n > 0);

    auto parsed = parse_hello_ack(buf.data(), static_cast<size_t>(n));
    REQUIRE(parsed.has_value());
    CHECK(parsed->width == 512);
    CHECK(parsed->height == 342);
    CHECK(parsed->byterate == 6000);
    CHECK(parsed->num_codecs == 2);

    close_sock(client_sock);
}

TEST_CASE("udp_transport — wait_for_hello")
{
    uint16_t port = test_port(5);

    //  Start a thread that will send a HELLO after a short delay
    std::thread sender(
        [port]
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            test_client client(port);
            hello_packet hello;
            hello.width = 512;
            hello.height = 342;
            hello.byterate = 6000;
            hello.num_codecs = 1;
            hello.codecs[0] = 0x02;
            client.send_bytes(serialize(hello));
        });

    udp_transport tp(port);
    CHECK_FALSE(tp.has_client());

    auto hello = tp.wait_for_hello();
    CHECK(tp.has_client());
    CHECK(hello.width == 512);
    CHECK(hello.height == 342);
    CHECK(hello.byterate == 6000);
    CHECK(hello.num_codecs == 1);
    CHECK(hello.codecs[0] == 0x02);

    sender.join();
}

TEST_CASE("udp_transport — ignores non-feedback packets")
{
    uint16_t port = test_port(6);
    udp_transport tp(port, "127.0.0.1", port + 100);

    //  Send a HELLO packet (wrong type) to the server's feedback port
    test_client client(port);
    hello_packet hello;
    hello.width = 512;
    hello.height = 342;
    client.send_bytes(serialize(hello));

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    //  Should be silently ignored
    CHECK_FALSE(tp.receive_feedback().has_value());
}
