#include "flimutil.hpp"

#include "arg_iterator.hpp"
#include "bitmap.hpp"
#include "errors.hpp"
#include "file_handle.hpp"
#include "flim.hpp"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <format>
#include <functional>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace macflim
{

static void print_summary(const flim &fl)
{
    //  Comment starts after "FLIM\n"
    const auto &comment = fl.comment();
    if (comment.size() > 5)
        std::cout << std::format("Comment: {}\n", comment.c_str() + 5);
    std::cout << std::format("Version: {}\n", fl.version());
    std::cout << std::format("Components: {}\n\n", fl.component_count());

    std::cout << std::format("{:<6}  {:<10}  {:<10}  {:<10}\n", "Index", "Type", "Offset", "Size");
    std::cout << std::format("{:<6}  {:<10}  {:<10}  {:<10}\n", "-----", "----------", "----------", "----------");
    for (size_t i = 0; i < fl.component_count(); i++)
    {
        auto &c = fl.component(i);
        std::cout << std::format("{:<6}  {:<10}  {:<10}  {:<10}\n", i, c.type_name(), c.offset, c.size);
    }
}

static void print_info(const flim &fl)
{
    auto *data = fl.find_component_data(component_type::info);
    if (!data)
        return;

    flim_info fi;
    fi.deserialize(data->data(), data->size());

    std::cout << std::format("\nInfo:\n");
    std::cout << std::format("  Dimensions: {}x{}\n", fi.width_, fi.height_);
    std::cout << std::format("  Silent: {}\n", fi.silent_ ? "yes" : "no");
    std::cout << std::format("  Frames: {}\n", fi.frame_count_);
    std::cout << std::format("  Total ticks: {}\n", fi.total_ticks_);
    std::cout << std::format("  Byterate: {}\n", fi.byterate_);
}

static void print_toc(const flim &fl)
{
    auto *data = fl.find_component_data(component_type::toc);
    if (!data)
        return;

    size_t frame_count = data->size() / 2;
    const uint8_t *tp = data->data();

    std::cout << std::format("\nTOC ({} frames):\n", frame_count);
    std::cout << std::format("  {:<8}  {:<10}  {:<10}\n", "Frame", "Size", "Offset");
    std::cout << std::format("  {:<8}  {:<10}  {:<10}\n", "--------", "----------", "----------");

    size_t offset = 0;
    for (size_t frame = 0; frame < frame_count; frame++)
    {
        uint16_t frame_size = read2(tp);
        std::cout << std::format("  {:<8}  {:<10}  {:<10}\n", frame, frame_size, offset);
        offset += frame_size;
    }
}

static bool extract_poster(const flim &fl, const std::string &outpath)
{
    auto *data = fl.find_component_data(component_type::poster);
    if (!data)
    {
        std::cerr << "No poster component found\n";
        return false;
    }

    //  Poster is a 1-bit packed bitmap (128x86, no header)
    size_t width = 128;
    size_t height = data->size() / (width / 8);

    bitmap fb(*data, width, height, false);
    write_grayscale(outpath.c_str(), fb.as_image());

    std::cout << std::format("\nPoster extracted to '{}' ({}x{})\n", outpath, width, height);
    return true;
}

static void dump_hex(const uint8_t *data, size_t size, std::string_view indent)
{
    for (size_t i = 0; i < size; i++)
    {
        if (i % 16 == 0)
            std::cout << std::format("{}{:04x}: ", indent, i);
        std::cout << std::format("{:02x} ", data[i]);
        if (i % 16 == 15 || i == size - 1)
            std::cout << std::format("\n");
    }
}

static void dump_sound_info(const frame &f)
{
    if (f.audio.empty())
    {
        std::cout << std::format("  Sound block size: 2 bytes\n");
        return;
    }

    size_t sound_block_size = f.ticks * 370 + 8;
    std::cout << std::format("  Sound block size: {} bytes\n", sound_block_size);
    std::cout << std::format("    ffMode: 0x0000\n");
    std::cout << std::format("    Rate: 65536 (0x00010000)\n");
    std::cout << std::format("    Audio data: {} bytes\n", f.audio.size());

    size_t bytes_per_tick = 370;
    size_t num_ticks = (f.audio.size() + bytes_per_tick - 1) / bytes_per_tick;
    for (size_t tick = 0; tick < num_ticks && tick < f.ticks; tick++)
    {
        size_t tick_offset = tick * bytes_per_tick;
        size_t tick_size =
            (tick_offset + bytes_per_tick <= f.audio.size()) ? bytes_per_tick : (f.audio.size() - tick_offset);

        std::cout << std::format("    tick {}: [", tick);
        size_t preview_bytes = std::min(tick_size, size_t{8});
        for (size_t i = 0; i < preview_bytes; i++)
            std::cout << std::format("{:02x} ", f.audio[tick_offset + i]);
        if (tick_size > preview_bytes)
            std::cout << std::format("... ");
        std::cout << std::format("] ({} bytes)\n", tick_size);
    }
}

static void dump_video_info(const frame &f)
{
    std::cout << std::format("  Video block size: {} bytes\n", f.video.size());
    if (f.video.size() < 4)
        return;

    uint8_t codec_sig = f.video[3];
    const char *codec_name = "unknown";
    switch (codec_sig)
    {
    case 0x00:
        codec_name = "null";
        break;
    case 0x01:
        codec_name = "z16";
        break;
    case 0x02:
        codec_name = "z32";
        break;
    case 0x03:
        codec_name = "invert";
        break;
    case 0x04:
        codec_name = "lines";
        break;
    }
    std::cout << std::format("  Codec: 0x{:02x} ({})\n", codec_sig, codec_name);

    size_t ops_size = f.video.size() - 4;
    std::cout << std::format("  Operations: {} bytes\n", ops_size);

    if (ops_size > 0)
    {
        std::cout << std::format("  Data:\n");
        dump_hex(f.video.data() + 4, ops_size, "    ");
    }
}

static void dump_frame_data(const std::vector<uint8_t> &frame_data, size_t frame_number, bool raw)
{
    std::cout << std::format("\nFrame {} (size: {} bytes):\n", frame_number, frame_data.size());

    if (raw)
    {
        dump_hex(frame_data.data(), frame_data.size(), "  ");
        return;
    }

    frame f = frame::deserialize(frame_data.data(), frame_data.size());
    std::cout << std::format("  Ticks: {}\n", f.ticks);
    dump_sound_info(f);
    dump_video_info(f);
}

static bool dump_frame(const flim &fl, size_t frame_number, bool raw)
{
    auto *toc_data = fl.find_component_data(component_type::toc);
    auto *movie_data = fl.find_component_data(component_type::movie);

    if (!toc_data)
    {
        std::cerr << std::format("No TOC component found\n");
        return false;
    }

    if (!movie_data)
    {
        std::cerr << std::format("No movie component found\n");
        return false;
    }

    size_t frame_count = toc_data->size() / 2;
    if (frame_number >= frame_count)
    {
        std::cerr << std::format("Frame {} out of range (total frames: {})\n", frame_number, frame_count);
        return false;
    }

    //  Calculate frame offset and size from TOC
    const uint8_t *tp = toc_data->data();
    size_t offset = 0;
    uint16_t frame_size = 0;

    for (size_t i = 0; i <= frame_number; i++)
    {
        frame_size = read2(tp);
        if (i < frame_number)
            offset += frame_size;
    }

    //  Extract frame data from movie blob
    std::vector<uint8_t> frame_data(movie_data->begin() + offset, movie_data->begin() + offset + frame_size);

    dump_frame_data(frame_data, frame_number, raw);

    return true;
}

static bool extract_initial(const flim &fl, const std::string &outpath)
{
    auto *data = fl.find_component_data(component_type::initial);
    if (!data)
    {
        std::cerr << std::format("No initial frame component found\n");
        return false;
    }

    //  Initial frame has a 6-byte header: type(2) + width(2) + height(2)
    if (data->size() < 6)
    {
        std::cerr << std::format("Initial component too small\n");
        return false;
    }
    const uint8_t *p = data->data();
    /*uint16_t type =*/read2(p); //  0x00 = bitmap
    uint16_t width = read2(p);
    uint16_t height = read2(p);

    std::vector<uint8_t> bitmap_data(p, p + (data->size() - 6));
    bitmap fb(bitmap_data, width, height, false);
    write_grayscale(outpath.c_str(), fb.as_image());

    std::cout << std::format("\nInitial frame extracted to '{}' ({}x{})\n", outpath, width, height);
    return true;
}

struct flimutil_options
{
    bool show_info = false;
    bool show_toc = false;
    std::string poster_outpath;
    std::string initial_outpath;
    int frame_number = -1;
    bool raw = false;
};

using flimutil_flag_handler = std::function<void(arg_iterator &, flimutil_options &)>;

static const std::unordered_map<std::string_view, flimutil_flag_handler> &flimutil_dispatch_table()
{
    static const std::unordered_map<std::string_view, flimutil_flag_handler> table = {
        {"--info", []([[maybe_unused]] arg_iterator &args, flimutil_options &opts) { opts.show_info = true; }},
        {"--toc", []([[maybe_unused]] arg_iterator &args, flimutil_options &opts) { opts.show_toc = true; }},
        {"--raw", []([[maybe_unused]] arg_iterator &args, flimutil_options &opts) { opts.raw = true; }},
        {"--poster", [](arg_iterator &args, flimutil_options &opts)
         { opts.poster_outpath = std::string(args.optional_value("out.pgm")); }},
        {"--initial", [](arg_iterator &args, flimutil_options &opts)
         { opts.initial_outpath = std::string(args.optional_value("out.pgm")); }},
        {"--frame", [](arg_iterator &args, flimutil_options &opts)
         { opts.frame_number = std::stoi(std::string(args.next_value())); }},
    };
    return table;
}

int flimutil_main(int argc, char **argv)
{
    try
    {
        std::string path = *argv;
        argc--;
        argv++;

        flimutil_options opts;
        const auto &dispatch = flimutil_dispatch_table();
        arg_iterator args(argc, argv);

        while (args.has_next())
        {
            auto arg = args.next();
            auto it = dispatch.find(arg);
            if (it == dispatch.end())
            {
                std::cerr << std::format("Unknown option '{}'\n", arg);
                return EXIT_FAILURE;
            }
            it->second(args, opts);
        }

        file_handle f;
        try
        {
            f = file_handle(path, "rb");
        }
        catch (const std::runtime_error &)
        {
            std::cerr << std::format("Cannot open '{}'\n", path);
            return EXIT_FAILURE;
        }

        flim fl;
        fl.read(f);

        print_summary(fl);

        if (opts.show_info)
            print_info(fl);

        if (opts.show_toc)
            print_toc(fl);

        if (!opts.poster_outpath.empty())
            extract_poster(fl, opts.poster_outpath);

        if (!opts.initial_outpath.empty())
            extract_initial(fl, opts.initial_outpath);

        if (opts.frame_number >= 0)
            dump_frame(fl, opts.frame_number, opts.raw);

        return EXIT_SUCCESS;
    }
    catch (const std::exception &error)
    {
        std::cerr << "**** ERROR: [" << error.what() << "]\n";
        return EXIT_FAILURE;
    }
}

} // namespace macflim
