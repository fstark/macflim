/// flimplayer — SDL2-based player for .flim files and macflim:// streaming.
///
/// Usage:
///   flimplayer <file.flim>              Local playback from a .flim file
///   flimplayer macflim://<host>[:<port>] Stream from a macflim server

#include "bitmap.hpp"
#include "decoder.hpp"
#include "file_handle.hpp"
#include "flim.hpp"
#include "frame.hpp"
#include "sdl_display.hpp"
#include "streaming/protocol.hpp"
#include "streaming/udp_client.hpp"

#include <array>
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

// ---------------------------------------------------------------------------
// Local .flim playback
// ---------------------------------------------------------------------------

namespace
{

/// Extract the initial screen bitmap from a flim, or return a blank screen.
bitmap extract_initial_screen(const flim &fl, const flim_info &info)
{
    auto *data = fl.find_component_data(component_type::initial);
    if (data && data->size() > 6)
    {
        const uint8_t *p = data->data();
        /*uint16_t type =*/read2(p);
        uint16_t width = read2(p);
        uint16_t height = read2(p);
        std::vector<uint8_t> bitmap_bytes(p, p + (data->size() - 6));
        return bitmap(bitmap_bytes, width, height, false);
    }

    bitmap blank(info.width, info.height);
    blank.fill(0xFF);
    return blank;
}

/// Parse the TOC component into a vector of {offset, size} pairs for each frame.
struct frame_loc
{
    size_t offset;
    size_t size;
};

std::vector<frame_loc> parse_toc(const std::vector<uint8_t> &toc_data)
{
    std::vector<frame_loc> locs;
    const uint8_t *p = toc_data.data();
    size_t offset = 0;
    size_t frame_count = toc_data.size() / 2;
    locs.reserve(frame_count);

    for (size_t i = 0; i < frame_count; ++i)
    {
        uint16_t frame_size = read2(p);
        locs.push_back({offset, frame_size});
        offset += frame_size;
    }
    return locs;
}

int play_flim(std::string_view path, bool no_sync = false)
{
    std::clog << std::format("Opening {}\n", path);

    file_handle fh(path, "rb");
    flim fl;
    fl.read(fh);

    //  Get info
    auto *info_data = fl.find_component_data(component_type::info);
    if (!info_data)
        throw std::runtime_error("Flim file has no info component");

    flim_info info;
    info.deserialize(info_data->data(), info_data->size());
    std::clog << std::format("{}x{}, {} frames, {} ticks, byterate {}\n", info.width, info.height, info.frame_count,
                             info.total_ticks, info.byterate);

    //  Get TOC and movie data
    auto *toc_data = fl.find_component_data(component_type::toc);
    auto *movie_data = fl.find_component_data(component_type::movie);
    if (!toc_data || !movie_data)
        throw std::runtime_error("Flim file missing TOC or movie component");

    auto frame_locs = parse_toc(*toc_data);
    std::clog << std::format("{} frames in TOC\n", frame_locs.size());

    //  Set up display
    bitmap screen = extract_initial_screen(fl, info);
    sdl_display display(info.width, info.height, 2, std::format("flimplayer — {}", path), !no_sync);
    display.update_screen(screen);

    //  Play frames at 60 Hz tick rate
    constexpr auto tick_duration = std::chrono::microseconds(1000000 / 60);
    auto next_tick = std::chrono::steady_clock::now();

    for (size_t i = 0; i < frame_locs.size(); ++i)
    {
        if (display.should_quit())
            break;

        //  Deserialize frame from movie blob
        auto &loc = frame_locs[i];
        frame f = frame::deserialize(movie_data->data() + loc.offset, loc.size);

        //  Apply video delta
        if (!f.video.empty())
            apply_delta(screen, f.video);

        //  Display
        display.update_screen(screen);

        //  Wait for the right number of ticks
        if (!no_sync)
        {
            size_t ticks = std::max<size_t>(f.ticks, 1);
            next_tick += tick_duration * ticks;
            std::this_thread::sleep_until(next_tick);
        }
    }

    //  Hold the last frame until the user closes the window
    while (!display.should_quit())
        std::this_thread::sleep_for(std::chrono::milliseconds(16));

    return EXIT_SUCCESS;
}

} // namespace

// ---------------------------------------------------------------------------
// Streaming playback (macflim:// protocol)
// ---------------------------------------------------------------------------

namespace
{

constexpr uint16_t DEFAULT_PORT = 5004;

/// Parse macflim://host[:port] into host and port components.
struct server_address
{
    std::string host;
    uint16_t port = DEFAULT_PORT;
};

server_address parse_macflim_url(std::string_view url)
{
    constexpr std::string_view prefix = "macflim://";
    auto authority = url.substr(prefix.size());

    server_address addr;
    auto colon = authority.rfind(':');
    if (colon != std::string_view::npos)
    {
        addr.host = std::string(authority.substr(0, colon));
        addr.port = static_cast<uint16_t>(std::stoi(std::string(authority.substr(colon + 1))));
    }
    else
    {
        addr.host = std::string(authority);
    }
    return addr;
}

/// Tracks which frame sequence numbers were received, producing FEEDBACK history bitmaps.
/// Bit layout matches protocol.hpp: bit N of the 128-bit field = (last_displayed_seq - N),
/// stored little-endian across bytes (bit 0 = LSB of byte 0).
struct frame_history
{
    uint32_t last_displayed = 0;
    std::array<uint8_t, HISTORY_BYTES> bits = {};

    void record(uint32_t seq)
    {
        if (last_displayed == 0)
        {
            //  First frame ever received
            last_displayed = seq;
            bits[0] = 0x01;
            return;
        }

        if (seq > last_displayed)
        {
            //  Normal case: new frame advances the window
            shift_left(seq - last_displayed);
            bits[0] |= 0x01;
            last_displayed = seq;
        }
        else if (seq < last_displayed)
        {
            //  Late arrival: still mark it in the history if in range
            uint32_t offset = last_displayed - seq;
            if (offset < HISTORY_BITS)
                bits[offset / 8] |= static_cast<uint8_t>(1u << (offset % 8));
        }
    }

    [[nodiscard]] feedback_packet make_feedback() const
    {
        feedback_packet fb;
        fb.last_displayed_seq = last_displayed;
        fb.history = bits;
        return fb;
    }

  private:
    void shift_left(uint32_t n)
    {
        if (n >= HISTORY_BITS)
        {
            bits.fill(0);
            return;
        }

        size_t byte_shift = n / 8;
        size_t bit_shift = n % 8;

        //  Shift whole bytes toward higher indices
        if (byte_shift > 0)
        {
            for (size_t i = HISTORY_BYTES; i-- > byte_shift;)
                bits[i] = bits[i - byte_shift];
            for (size_t i = 0; i < byte_shift && i < HISTORY_BYTES; ++i)
                bits[i] = 0;
        }

        //  Shift remaining bits within bytes
        if (bit_shift > 0)
        {
            for (size_t i = HISTORY_BYTES; i-- > 1;)
                bits[i] = static_cast<uint8_t>((bits[i] << bit_shift) | (bits[i - 1] >> (8 - bit_shift)));
            bits[0] = static_cast<uint8_t>(bits[0] << bit_shift);
        }
    }
};

/// Build a HELLO packet advertising our capabilities.
hello_packet build_hello()
{
    hello_packet hello;
    hello.width = 512;
    hello.height = 342;
    hello.byterate = 6000;
    hello.dither = 0;
    hello.num_codecs = 5;
    hello.codecs = {0x00, 0x01, 0x02, 0x03, 0x04}; // null, z16, z32, invert, lines
    return hello;
}

/// Drop statistics tracked during streaming.
struct drop_stats
{
    uint32_t received = 0;    // total FRAME packets pulled from socket
    uint32_t dropped = 0;     // frames discarded because SPACE was held
    uint32_t seq_gaps = 0;    // sequence number gaps (frames we never received)
    uint32_t highest_seq = 0; // highest sequence number seen so far
    uint32_t feedbacks_sent = 0;
    uint32_t feedbacks_suppressed = 0; // feedbacks not sent because SPACE was held
};

int stream_flim(std::string_view url, bool no_sync)
{
    auto addr = parse_macflim_url(url);
    std::clog << std::format("Connecting to {}:{}\n", addr.host, addr.port);

    //  Open UDP socket and perform handshake
    udp_client client(addr.host, addr.port);
    client.send_hello(build_hello());
    std::clog << "HELLO sent, waiting for ACK...\n";

    auto ack = client.wait_for_hello_ack();
    std::clog << std::format("Session: {}x{}, byterate {}, {} codecs\n", ack.width, ack.height, ack.byterate,
                             ack.num_codecs);

    //  Set up display — start with all-black (Mac convention: 0xFF = black)
    bitmap screen(ack.width, ack.height);
    screen.fill(0xFF);
    sdl_display display(ack.width, ack.height, 2, std::format("flimplayer — {}:{}", addr.host, addr.port), !no_sync);
    display.update_screen(screen);

    //  Streaming receive loop
    frame_history history;
    drop_stats stats;
    uint32_t frames_displayed = 0;
    constexpr auto feedback_interval = std::chrono::milliseconds(100);
    auto next_feedback = std::chrono::steady_clock::now() + feedback_interval;

    std::clog << "SPACE = simulate packet loss, RETURN = log drop stats\n";

    while (!display.should_quit())
    {
        //  Sample keyboard state for interactive controls
        const uint8_t *keys = SDL_GetKeyboardState(nullptr);
        bool dropping = keys[SDL_SCANCODE_SPACE] != 0;
        bool log_stats = keys[SDL_SCANCODE_RETURN] != 0;

        //  Drain all available FRAME packets
        bool got_frame = false;
        while (auto pkt = client.receive_packet())
        {
            auto fv = parse_frame(pkt->data(), pkt->size());
            if (!fv)
                continue;

            ++stats.received;

            //  Track sequence gaps (frames the network lost before reaching us)
            if (stats.highest_seq > 0 && fv->header.seq > stats.highest_seq + 1)
                stats.seq_gaps += (fv->header.seq - stats.highest_seq - 1);
            if (fv->header.seq > stats.highest_seq)
                stats.highest_seq = fv->header.seq;

            //  SPACE held: simulate client-side packet loss
            if (dropping)
            {
                ++stats.dropped;
                continue;
            }

            //  Apply video delta to screen
            if (fv->video_len > 0)
            {
                std::vector<uint8_t> video(fv->video_data, fv->video_data + fv->video_len);
                apply_delta(screen, video);
            }

            history.record(fv->header.seq);
            ++frames_displayed;
            got_frame = true;
        }

        //  Update display if we got new frames
        if (got_frame)
            display.update_screen(screen);

        //  Send feedback periodically (suppress while dropping to starve the server too)
        auto now = std::chrono::steady_clock::now();
        if (history.last_displayed > 0 && now >= next_feedback)
        {
            if (dropping)
            {
                ++stats.feedbacks_suppressed;
            }
            else
            {
                client.send_feedback(history.make_feedback());
                ++stats.feedbacks_sent;
            }
            next_feedback = now + feedback_interval;
        }

        //  RETURN held: log drop statistics
        if (log_stats)
            std::clog << std::format("drops: {} client-side ({} SPACE-dropped + {} network-lost), "
                                     "{} feedbacks sent / {} suppressed\n",
                                     stats.dropped + stats.seq_gaps, stats.dropped, stats.seq_gaps,
                                     stats.feedbacks_sent, stats.feedbacks_suppressed);

        //  Brief sleep to avoid busy-spinning when no frames arrive
        if (!got_frame)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    std::clog << std::format("Done: {} frames displayed, {} received, {} SPACE-dropped, {} network-lost\n",
                             frames_displayed, stats.received, stats.dropped, stats.seq_gaps);
    return EXIT_SUCCESS;
}

} // namespace

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int flimplayer_main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::cerr << "Usage: flimplayer [--no-sync] <file.flim | macflim://host[:port]>\n";
        return EXIT_FAILURE;
    }

    bool no_sync = false;
    std::string_view arg;

    for (int i = 1; i < argc; ++i)
    {
        std::string_view a = argv[i];
        if (a == "--no-sync")
            no_sync = true;
        else
            arg = a;
    }

    if (arg.empty())
    {
        std::cerr << "Usage: flimplayer [--no-sync] <file.flim | macflim://host[:port]>\n";
        return EXIT_FAILURE;
    }

    try
    {
        if (arg.starts_with("macflim://"))
            return stream_flim(arg, no_sync);

        //  Local .flim file playback
        return play_flim(arg, no_sync);
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
    return macflim::flimplayer_main(argc, argv);
}
