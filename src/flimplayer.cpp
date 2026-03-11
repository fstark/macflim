/// flimplayer — SDL2-based player for .flim files and macflim:// streaming.
///
/// Usage:
///   flimplayer <file.flim>              Local playback from a .flim file
///   flimplayer macflim://<host>[:<port>] Streaming from a macflim server (TODO)

#include "bitmap.hpp"
#include "decoder.hpp"
#include "file_handle.hpp"
#include "flim.hpp"
#include "frame.hpp"
#include "sdl_display.hpp"

#include <chrono>
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
        {
            std::cerr << "Streaming mode not yet implemented.\n";
            return EXIT_FAILURE;
        }

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
