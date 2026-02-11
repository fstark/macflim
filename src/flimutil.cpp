#include "flimformat_types.hpp"
#include "framebuffer.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

static const size_t COMMENT_SIZE = 1022;
static const size_t CHECKSUM_SIZE = 2;

struct component_entry
{
    uint16_t type;
    uint32_t offset;
    uint32_t size;
};

struct flim_header
{
    std::string comment;
    uint16_t checksum;
    uint16_t version;
    std::vector<component_entry> components;
    long data_start; //  File offset where component data begins
};

static bool read_header(FILE *f, flim_header &hdr)
{
    //  Read comment block (1022 bytes, starts with "FLIM\n")
    char comment[COMMENT_SIZE + 1];
    if (fread(comment, 1, COMMENT_SIZE, f) != COMMENT_SIZE)
    {
        fprintf(stderr, "Failed to read comment block\n");
        return false;
    }
    comment[COMMENT_SIZE] = 0;

    if (memcmp(comment, "FLIM\n", 5) != 0)
    {
        fprintf(stderr, "Not a valid flim file (bad signature)\n");
        return false;
    }
    hdr.comment = comment + 5;

    //  Read checksum (2 bytes, big-endian)
    uint8_t checksum_bytes[CHECKSUM_SIZE];
    if (fread(checksum_bytes, 1, CHECKSUM_SIZE, f) != CHECKSUM_SIZE)
    {
        fprintf(stderr, "Failed to read checksum\n");
        return false;
    }
    const uint8_t *cp = checksum_bytes;
    hdr.checksum = read2(cp);

    //  Read header: version (2 bytes) + component count (2 bytes)
    uint8_t header_prefix[4];
    if (fread(header_prefix, 1, 4, f) != 4)
    {
        fprintf(stderr, "Failed to read header\n");
        return false;
    }
    const uint8_t *hp = header_prefix;
    hdr.version = read2(hp);
    uint16_t component_count = read2(hp);

    //  Read component directory: 10 bytes per entry (type:2 + offset:4 + size:4)
    hdr.components.resize(component_count);
    std::vector<uint8_t> dir_data(component_count * 10);
    if (fread(dir_data.data(), 1, dir_data.size(), f) != dir_data.size())
    {
        fprintf(stderr, "Failed to read component directory\n");
        return false;
    }

    const uint8_t *dp = dir_data.data();
    for (int i = 0; i < component_count; i++)
    {
        hdr.components[i].type = read2(dp);
        hdr.components[i].offset = read4(dp);
        hdr.components[i].size = read4(dp);
    }

    hdr.data_start = COMMENT_SIZE + CHECKSUM_SIZE + 4 + component_count * 10;

    return true;
}

static void print_summary(const flim_header &hdr)
{
    printf("Comment: %s\n", hdr.comment.c_str());
    printf("Checksum: 0x%04x\n", hdr.checksum);
    printf("Version: %d\n", hdr.version);
    printf("Components: %zu\n\n", hdr.components.size());

    printf("%-6s  %-10s  %-10s  %-10s\n", "Index", "Type", "Offset", "Size");
    printf("%-6s  %-10s  %-10s  %-10s\n", "-----", "----------", "----------", "----------");
    for (size_t i = 0; i < hdr.components.size(); i++)
    {
        printf("%-6zu  %-10s  %-10u  %-10u\n",
               i,
               component_type_name(hdr.components[i].type),
               hdr.components[i].offset,
               hdr.components[i].size);
    }
}

static void print_info(FILE *f, const flim_header &hdr)
{
    for (auto &c : hdr.components)
    {
        if (c.type != component_info)
            continue;

        std::vector<uint8_t> data(c.size);
        fseek(f, hdr.data_start + c.offset, SEEK_SET);
        if (fread(data.data(), 1, data.size(), f) != data.size())
        {
            fprintf(stderr, "Failed to read info component\n");
            continue;
        }

        flim_info fi;
        fi.deserialize(data.data(), data.size());

        printf("\nInfo:\n");
        printf("  Dimensions: %zux%zu\n", fi.width_, fi.height_);
        printf("  Silent: %s\n", fi.silent_ ? "yes" : "no");
        printf("  Frames: %zu\n", fi.frame_count_);
        printf("  Total ticks: %zu\n", fi.total_ticks_);
        printf("  Byterate: %zu\n", fi.byterate_);
    }
}

static void print_toc(FILE *f, const flim_header &hdr)
{
    for (auto &c : hdr.components)
    {
        if (c.type != component_toc)
            continue;

        std::vector<uint8_t> data(c.size);
        fseek(f, hdr.data_start + c.offset, SEEK_SET);
        if (fread(data.data(), 1, data.size(), f) != data.size())
        {
            fprintf(stderr, "Failed to read toc component\n");
            continue;
        }

        size_t frame_count = c.size / 2;
        const uint8_t *tp = data.data();

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
}

static bool extract_poster(FILE *f, const flim_header &hdr, const std::string &outpath)
{
    for (auto &c : hdr.components)
    {
        if (c.type != component_poster)
            continue;

        std::vector<uint8_t> data(c.size);
        fseek(f, hdr.data_start + c.offset, SEEK_SET);
        if (fread(data.data(), 1, data.size(), f) != data.size())
        {
            fprintf(stderr, "Failed to read poster component\n");
            return false;
        }

        //  Poster is a 1-bit packed bitmap (128x86, no header)
        size_t width = 128;
        size_t height = c.size / (width / 8);

        framebuffer fb(data, width, height, false);
        write_image(outpath.c_str(), fb.as_image());

        printf("\nPoster extracted to '%s' (%zux%zu)\n", outpath.c_str(), width, height);
        return true;
    }

    fprintf(stderr, "No poster component found\n");
    return false;
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
    
    if (frame_data.size() < 4)
    {
        printf("  Frame too small to parse\n");
        return;
    }
    
    const uint8_t *p = frame_data.data();
    const uint8_t *end = p + frame_data.size();
    
    //  Read tick count (2 bytes)
    uint16_t ticks = read2(p);
    printf("  Ticks: %u\n", ticks);
    
    //  Read sound block size (2 bytes)
    uint16_t sound_size = read2(p);
    printf("  Sound block size: %u bytes\n", sound_size);
    
    if (sound_size > 2)
    {
        //  Read sound header
        if (p + 6 > end)
        {
            printf("  Error: truncated sound header\n");
            return;
        }
        uint16_t ff_mode = read2(p);
        uint32_t rate = read4(p);
        printf("    ffMode: 0x%04x\n", ff_mode);
        printf("    Rate: %u (0x%08x)\n", rate, rate);
        
        size_t audio_data_size = sound_size - 8;  // subtract header size
        printf("    Audio data: %zu bytes\n", audio_data_size);
        
        //  Display audio data as ticks (370 bytes per tick)
        size_t bytes_per_tick = 370;
        size_t num_ticks = (audio_data_size + bytes_per_tick - 1) / bytes_per_tick;
        for (size_t tick = 0; tick < num_ticks && tick < ticks; tick++)
        {
            size_t tick_offset = tick * bytes_per_tick;
            size_t tick_size = (tick_offset + bytes_per_tick <= audio_data_size) ? bytes_per_tick : (audio_data_size - tick_offset);
            
            printf("    tick %zu: [", tick);
            size_t preview_bytes = tick_size < 8 ? tick_size : 8;
            for (size_t i = 0; i < preview_bytes; i++)
            {
                if (p + tick_offset + i < end)
                    printf("%02x ", p[tick_offset + i]);
            }
            if (tick_size > preview_bytes)
                printf("... ");
            printf("] (%zu bytes)\n", tick_size);
        }
        
        //  Skip audio data
        p += audio_data_size;
    }
    
    //  Read video block size (2 bytes)
    if (p + 2 > end)
    {
        printf("  Error: truncated video size\n");
        return;
    }
    uint16_t video_size_plus_two = read2(p);
    size_t video_size = video_size_plus_two > 2 ? video_size_plus_two - 2 : 0;
    printf("  Video block size: %zu bytes\n", video_size);
    
    //  Parse video data structure
    if (video_size > 0)
    {
        if (p + 4 > end)
        {
            printf("  Error: truncated video header\n");
            return;
        }
        
        //  Skip 3 bytes of padding
        p += 3;
        
        //  Read codec signature
        uint8_t codec_sig = *p++;
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
        
        //  Display remaining data as operations
        size_t ops_size = video_size - 4;  // subtract header
        printf("  Operations: %zu bytes\n", ops_size);
        
        if (ops_size > 0)
        {
            printf("  Data:\n");
            for (size_t i = 0; i < ops_size && p + i < end; i++)
            {
                if (i % 16 == 0)
                    printf("    %04zx: ", i);
                printf("%02x ", p[i]);
                if (i % 16 == 15 || i == ops_size - 1)
                    printf("\n");
            }
        }
    }
}

static bool dump_frame(FILE *f, const flim_header &hdr, size_t frame_number, bool raw)
{
    //  First, find the TOC component to get frame offsets
    const component_entry *toc_comp = nullptr;
    const component_entry *movie_comp = nullptr;

    for (auto &c : hdr.components)
    {
        if (c.type == component_toc)
            toc_comp = &c;
        else if (c.type == component_movie)
            movie_comp = &c;
    }

    if (!toc_comp)
    {
        fprintf(stderr, "No TOC component found\n");
        return false;
    }

    if (!movie_comp)
    {
        fprintf(stderr, "No movie component found\n");
        return false;
    }

    //  Read TOC data
    std::vector<uint8_t> toc_data(toc_comp->size);
    fseek(f, hdr.data_start + toc_comp->offset, SEEK_SET);
    if (fread(toc_data.data(), 1, toc_data.size(), f) != toc_data.size())
    {
        fprintf(stderr, "Failed to read TOC component\n");
        return false;
    }

    size_t frame_count = toc_comp->size / 2;
    if (frame_number >= frame_count)
    {
        fprintf(stderr, "Frame %zu out of range (total frames: %zu)\n", frame_number, frame_count);
        return false;
    }

    //  Calculate frame offset and size from TOC
    const uint8_t *tp = toc_data.data();
    size_t offset = 0;
    uint16_t frame_size = 0;

    for (size_t i = 0; i <= frame_number; i++)
    {
        frame_size = read2(tp);
        if (i < frame_number)
            offset += frame_size;
    }

    //  Read frame data from movie component
    std::vector<uint8_t> frame_data(frame_size);
    fseek(f, hdr.data_start + movie_comp->offset + offset, SEEK_SET);
    if (fread(frame_data.data(), 1, frame_data.size(), f) != frame_data.size())
    {
        fprintf(stderr, "Failed to read frame data\n");
        return false;
    }

    //  Dump the frame data
    dump_frame_data(frame_data, frame_number, raw);

    return true;
}

static bool extract_initial(FILE *f, const flim_header &hdr, const std::string &outpath)
{
    for (auto &c : hdr.components)
    {
        if (c.type != component_initial)
            continue;

        std::vector<uint8_t> data(c.size);
        fseek(f, hdr.data_start + c.offset, SEEK_SET);
        if (fread(data.data(), 1, data.size(), f) != data.size())
        {
            fprintf(stderr, "Failed to read initial component\n");
            return false;
        }

        //  Initial frame has a 6-byte header: type(2) + width(2) + height(2)
        if (c.size < 6)
        {
            fprintf(stderr, "Initial component too small\n");
            return false;
        }
        const uint8_t *p = data.data();
        /*uint16_t type =*/read2(p); //  0x00 = framebuffer
        uint16_t width = read2(p);
        uint16_t height = read2(p);

        std::vector<uint8_t> bitmap(p, p + (c.size - 6));
        framebuffer fb(bitmap, width, height, false);
        write_image(outpath.c_str(), fb.as_image());

        printf("\nInitial frame extracted to '%s' (%ux%u)\n", outpath.c_str(), width, height);
        return true;
    }

    fprintf(stderr, "No initial frame component found\n");
    return false;
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

    flim_header hdr;
    if (!read_header(f, hdr))
    {
        fclose(f);
        return EXIT_FAILURE;
    }

    print_summary(hdr);

    if (show_info)
        print_info(f, hdr);

    if (show_toc)
        print_toc(f, hdr);

    if (!poster_outpath.empty())
        extract_poster(f, hdr, poster_outpath);

    if (!initial_outpath.empty())
        extract_initial(f, hdr, initial_outpath);

    if (frame_number >= 0)
        dump_frame(f, hdr, frame_number, raw);

    fclose(f);
    return EXIT_SUCCESS;
}
