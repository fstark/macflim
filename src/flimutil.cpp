#include "flim.hpp"
#include "framebuffer.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

static void print_summary(const flim &fl)
{
    //  Comment starts after "FLIM\n"
    const auto &comment = fl.comment();
    if (comment.size() > 5)
        printf("Comment: %s\n", comment.c_str() + 5);
    printf("Version: %d\n", fl.version());
    printf("Components: %zu\n\n", fl.component_count());

    printf("%-6s  %-10s  %-10s  %-10s\n", "Index", "Type", "Offset", "Size");
    printf("%-6s  %-10s  %-10s  %-10s\n", "-----", "----------", "----------", "----------");
    for (size_t i = 0; i < fl.component_count(); i++)
    {
        auto &c = fl.component(i);
        printf("%-6zu  %-10s  %-10u  %-10u\n",
               i, c.type_name(), c.offset, c.size);
    }
}

static void print_info(const flim &fl)
{
    auto *data = fl.find_component_data(component_info);
    if (!data)
        return;

    flim_info fi;
    fi.deserialize(data->data(), data->size());

    printf("\nInfo:\n");
    printf("  Dimensions: %zux%zu\n", fi.width_, fi.height_);
    printf("  Silent: %s\n", fi.silent_ ? "yes" : "no");
    printf("  Frames: %zu\n", fi.frame_count_);
    printf("  Total ticks: %zu\n", fi.total_ticks_);
    printf("  Byterate: %zu\n", fi.byterate_);
}

static void print_toc(const flim &fl)
{
    auto *data = fl.find_component_data(component_toc);
    if (!data)
        return;

    size_t frame_count = data->size() / 2;
    const uint8_t *tp = data->data();

    printf("\nTOC (%zu frames):\n", frame_count);
    printf("  %-8s  %-10s  %-10s\n", "Frame", "Size", "Offset");
    printf("  %-8s  %-10s  %-10s\n", "--------", "----------", "----------");

    size_t offset = 0;
    for (size_t frame = 0; frame < frame_count; frame++)
    {
        uint16_t frame_size = read2(tp);
        printf("  %-8zu  %-10u  %-10zu\n", frame, frame_size, offset);
        offset += frame_size;
    }
}

static bool extract_poster(const flim &fl, const std::string &outpath)
{
    auto *data = fl.find_component_data(component_poster);
    if (!data)
    {
        fprintf(stderr, "No poster component found\n");
        return false;
    }

    //  Poster is a 1-bit packed bitmap (128x86, no header)
    size_t width = 128;
    size_t height = data->size() / (width / 8);

    framebuffer fb(*data, width, height, false);
    write_image(outpath.c_str(), fb.as_image());

    printf("\nPoster extracted to '%s' (%zux%zu)\n", outpath.c_str(), width, height);
    return true;
}

static void dump_frame_data(const std::vector<uint8_t> &frame_data, size_t frame_number, bool raw)
{
    printf("\nFrame %zu (size: %zu bytes):\n", frame_number, frame_data.size());
    
    //  Raw mode: just dump hex bytes
    if (raw)
    {
        for (size_t i = 0; i < frame_data.size(); i++)
        {
            if (i % 16 == 0)
                printf("  %04zx: ", i);
            printf("%02x ", frame_data[i]);
            if (i % 16 == 15 || i == frame_data.size() - 1)
                printf("\n");
        }
        return;
    }
    
    frame f = frame::deserialize(frame_data.data(), frame_data.size());

    printf("  Ticks: %zu\n", f.ticks);

    //  Sound
    if (f.audio.empty())
    {
        printf("  Sound block size: 2 bytes\n");
    }
    else
    {
        size_t sound_block_size = f.ticks * 370 + 8;
        printf("  Sound block size: %zu bytes\n", sound_block_size);
        printf("    ffMode: 0x0000\n");
        printf("    Rate: 65536 (0x00010000)\n");
        printf("    Audio data: %zu bytes\n", f.audio.size());

        size_t bytes_per_tick = 370;
        size_t num_ticks = (f.audio.size() + bytes_per_tick - 1) / bytes_per_tick;
        for (size_t tick = 0; tick < num_ticks && tick < f.ticks; tick++)
        {
            size_t tick_offset = tick * bytes_per_tick;
            size_t tick_size = (tick_offset + bytes_per_tick <= f.audio.size()) ? bytes_per_tick : (f.audio.size() - tick_offset);

            printf("    tick %zu: [", tick);
            size_t preview_bytes = tick_size < 8 ? tick_size : 8;
            for (size_t i = 0; i < preview_bytes; i++)
                printf("%02x ", f.audio[tick_offset + i]);
            if (tick_size > preview_bytes)
                printf("... ");
            printf("] (%zu bytes)\n", tick_size);
        }
    }

    //  Video
    printf("  Video block size: %zu bytes\n", f.video.size());

    if (f.video.size() >= 4)
    {
        uint8_t codec_sig = f.video[3];
        const char *codec_name = "unknown";
        switch (codec_sig)
        {
            case 0x00: codec_name = "null"; break;
            case 0x01: codec_name = "z16"; break;
            case 0x02: codec_name = "z32"; break;
            case 0x03: codec_name = "invert"; break;
            case 0x04: codec_name = "lines"; break;
        }
        printf("  Codec: 0x%02x (%s)\n", codec_sig, codec_name);

        size_t ops_size = f.video.size() - 4;
        printf("  Operations: %zu bytes\n", ops_size);

        if (ops_size > 0)
        {
            printf("  Data:\n");
            for (size_t i = 0; i < ops_size; i++)
            {
                if (i % 16 == 0)
                    printf("    %04zx: ", i);
                printf("%02x ", f.video[4 + i]);
                if (i % 16 == 15 || i == ops_size - 1)
                    printf("\n");
            }
        }
    }
}

static bool dump_frame(const flim &fl, size_t frame_number, bool raw)
{
    auto *toc_data = fl.find_component_data(component_toc);
    auto *movie_data = fl.find_component_data(component_movie);

    if (!toc_data)
    {
        fprintf(stderr, "No TOC component found\n");
        return false;
    }

    if (!movie_data)
    {
        fprintf(stderr, "No movie component found\n");
        return false;
    }

    size_t frame_count = toc_data->size() / 2;
    if (frame_number >= frame_count)
    {
        fprintf(stderr, "Frame %zu out of range (total frames: %zu)\n", frame_number, frame_count);
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
    std::vector<uint8_t> frame_data(movie_data->begin() + offset,
                                    movie_data->begin() + offset + frame_size);

    dump_frame_data(frame_data, frame_number, raw);

    return true;
}

static bool extract_initial(const flim &fl, const std::string &outpath)
{
    auto *data = fl.find_component_data(component_initial);
    if (!data)
    {
        fprintf(stderr, "No initial frame component found\n");
        return false;
    }

    //  Initial frame has a 6-byte header: type(2) + width(2) + height(2)
    if (data->size() < 6)
    {
        fprintf(stderr, "Initial component too small\n");
        return false;
    }
    const uint8_t *p = data->data();
    /*uint16_t type =*/read2(p); //  0x00 = framebuffer
    uint16_t width = read2(p);
    uint16_t height = read2(p);

    std::vector<uint8_t> bitmap(p, p + (data->size() - 6));
    framebuffer fb(bitmap, width, height, false);
    write_image(outpath.c_str(), fb.as_image());

    printf("\nInitial frame extracted to '%s' (%ux%u)\n", outpath.c_str(), width, height);
    return true;
}

int flimutil_main(int argc, char **argv)
{
    std::string path = *argv;
    argc--;
    argv++;

    //  Parse options
    bool show_info = false;
    bool show_toc = false;
    std::string poster_outpath;
    std::string initial_outpath;
    int frame_number = -1;
    bool raw = false;

    while (argc)
    {
        if (!strcmp(*argv, "--info"))
            show_info = true;
        else if (!strcmp(*argv, "--toc"))
            show_toc = true;
        else if (!strcmp(*argv, "--poster"))
        {
            argc--;
            argv++;
            if (!argc || (*argv)[0] == '-')
            {
                poster_outpath = "out.pgm";
                continue;
            }
            poster_outpath = *argv;
        }
        else if (!strcmp(*argv, "--initial"))
        {
            argc--;
            argv++;
            if (!argc || (*argv)[0] == '-')
            {
                initial_outpath = "out.pgm";
                continue;
            }
            initial_outpath = *argv;
        }
        else if (!strcmp(*argv, "--frame"))
        {
            argc--;
            argv++;
            if (!argc)
            {
                fprintf(stderr, "--frame requires a frame number\n");
                return EXIT_FAILURE;
            }
            frame_number = atoi(*argv);
        }
        else if (!strcmp(*argv, "--raw"))
            raw = true;
        else
        {
            fprintf(stderr, "Unknown option '%s'\n", *argv);
            return EXIT_FAILURE;
        }
        argc--;
        argv++;
    }

    FILE *f = fopen(path.c_str(), "rb");
    if (!f)
    {
        fprintf(stderr, "Cannot open '%s'\n", path.c_str());
        return EXIT_FAILURE;
    }

    flim fl;
    if (!fl.read(f))
    {
        fclose(f);
        return EXIT_FAILURE;
    }
    fclose(f);

    print_summary(fl);

    if (show_info)
        print_info(fl);

    if (show_toc)
        print_toc(fl);

    if (!poster_outpath.empty())
        extract_poster(fl, poster_outpath);

    if (!initial_outpath.empty())
        extract_initial(fl, initial_outpath);

    if (frame_number >= 0)
        dump_frame(fl, frame_number, raw);

    return EXIT_SUCCESS;
}
