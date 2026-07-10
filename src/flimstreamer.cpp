/// flimstreamer — streaming server for .flim files over UDP.
///
/// Usage:
///   flimstreamer [--port <port>] [--byterate <n>] <file.flim>
///
/// Loads a .flim file, waits for a client HELLO on the given UDP port (default 5004),
/// then streams the pre-encoded frames to the client at 60Hz, processing feedback
/// to maintain pixel-perfect client state tracking and adaptive rate control.

#include "codec_spec.hpp"
#include "flim.hpp"
#include "streaming/flim_source.hpp"
#include "streaming/protocol.hpp"
#include "streaming/streaming_session.hpp"
#include "streaming/udp_transport.hpp"

#include <chrono>
#include <csignal>
#include <cstdlib>
#include <format>
#include <iostream>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace macflim
{

namespace
{

volatile std::sig_atomic_t g_running = 1;

void signal_handler(int /*sig*/)
{
    g_running = 0;
}

constexpr uint16_t DEFAULT_PORT = 5004;

struct server_config
{
    uint16_t port = DEFAULT_PORT;
    size_t max_byterate = 0; // 0 = use flim/client minimum
    std::string flim_path;
};

server_config parse_args(int argc, char **argv)
{
    server_config config;
    for (int i = 1; i < argc; ++i)
    {
        std::string_view arg = argv[i];
        if (arg == "--port" && i + 1 < argc)
            config.port = static_cast<uint16_t>(std::stoi(argv[++i]));
        else if (arg == "--byterate" && i + 1 < argc)
            config.max_byterate = static_cast<size_t>(std::stoi(argv[++i]));
        else if (!arg.starts_with("-"))
            config.flim_path = arg;
    }

    if (config.flim_path.empty())
        throw std::runtime_error("Usage: flimstreamer [--port <port>] [--byterate <n>] <file.flim>");

    return config;
}

/// Read flim info (dimensions, byterate) without decoding frames.
flim_info read_flim_info(std::string_view path)
{
    file_handle fh(path, "rb");
    flim fl;
    fl.read(fh);

    auto *info_data = fl.find_component_data(component_type::info);
    if (!info_data)
        throw std::runtime_error("Flim file has no info component");

    flim_info info;
    info.deserialize(info_data->data(), info_data->size());
    return info;
}

/// Build codec specs from the client's HELLO request.
std::vector<codec_spec> build_codecs(const hello_packet &hello, size_t width, size_t height)
{
    std::vector<codec_spec> codecs;
    for (size_t i = 0; i < hello.num_codecs; ++i)
    {
        uint8_t sig = hello.codecs[i];
        for (const auto &entry : codec_table)
        {
            if (entry.signature == sig)
            {
                codecs.push_back({entry.signature, entry.penalty, entry.factory(width, height)});
                break;
            }
        }
    }

    //  Fallback: if client requested nothing usable, use z32
    if (codecs.empty())
        codecs.push_back(make_codec("z32", width, height));

    return codecs;
}

/// Send HELLO_ACK echoing back the agreed parameters.
void send_handshake_response(udp_transport &tp, const flim_info &info, const std::vector<codec_spec> &codecs)
{
    hello_ack_packet ack;
    ack.width = info.width;
    ack.height = info.height;
    ack.byterate = info.byterate;
    ack.num_codecs = static_cast<uint8_t>(std::min(codecs.size(), MAX_CODECS));
    for (size_t i = 0; i < ack.num_codecs; ++i)
        ack.codecs[i] = codecs[i].signature;
    tp.send_hello_ack(ack);
}

int run_server(const server_config &config)
{
    if (config.max_byterate > 0)
        std::clog << std::format("Max byterate override: {}\n", config.max_byterate);
    std::clog << std::format("Loading {}\n", config.flim_path);
    auto info = read_flim_info(config.flim_path);
    std::clog << std::format("{}x{}, {} frames, byterate {}\n", info.width, info.height, info.frame_count,
                             info.byterate);

    //  Bind UDP socket and wait for a client
    auto tp = std::make_unique<udp_transport>(config.port);
    std::clog << std::format("Listening on UDP port {}...\n", config.port);

    auto hello = tp->wait_for_hello();
    std::clog << std::format("Client connected: {}x{}, byterate {}, {} codecs\n", hello.width, hello.height,
                             hello.byterate, hello.num_codecs);

    //  Build codecs and send ACK
    auto codecs = build_codecs(hello, info.width, info.height);
    send_handshake_response(*tp, info, codecs);
    std::clog << std::format("Session started with {} codecs\n", codecs.size());

    //  Decode .flim into target bitmaps
    auto source = make_flim_source(config.flim_path);

    //  Use the smallest of server-configured, client-requested, and flim-native byterate
    size_t byterate = std::min<size_t>(hello.byterate, info.byterate);
    if (config.max_byterate > 0)
        byterate = std::min(byterate, config.max_byterate);
    std::clog << std::format("Using byterate {}\n", byterate);

    //  Create session and stream at 60Hz
    streaming_session session(std::move(source), std::move(codecs), byterate, std::move(tp), info.width, info.height);

    constexpr auto tick_duration = std::chrono::microseconds(1000000 / 60);
    auto next_tick = std::chrono::steady_clock::now();

    while (g_running)
    {
        auto result = session.step();
        if (result == step_result::finished)
        {
            std::clog << "Stream finished.\n";
            break;
        }

        auto s = session.stats();
        std::clog << std::format("\rFrame {}, byterate {}, in-flight {}, dropped {}   ", s.frames_sent,
                                 s.current_byterate, s.in_flight, s.frames_dropped);

        next_tick += tick_duration;
        std::this_thread::sleep_until(next_tick);
    }

    auto final_stats = session.stats();
    std::clog << std::format("\nDone: {} frames sent, final byterate {}, dropped {}\n", final_stats.frames_sent,
                             final_stats.current_byterate, final_stats.frames_dropped);

    return EXIT_SUCCESS;
}

} // namespace

int flimstreamer_main(int argc, char **argv)
{
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    try
    {
        auto config = parse_args(argc, argv);
        return run_server(config);
    }
    catch (const std::exception &e)
    {
        std::cerr << std::format("Error: {}\n", e.what());
        return EXIT_FAILURE;
    }
}

} // namespace macflim

int main(int argc, char **argv)
{
    return macflim::flimstreamer_main(argc, argv);
}
