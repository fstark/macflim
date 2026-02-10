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
    long data_start;        //  File offset where component data begins
};

static bool read_header( FILE *f, flim_header &hdr )
{
    //  Read comment block (1022 bytes, starts with "FLIM\n")
    char comment[COMMENT_SIZE + 1];
    if (fread( comment, 1, COMMENT_SIZE, f ) != COMMENT_SIZE)
    {
        fprintf( stderr, "Failed to read comment block\n" );
        return false;
    }
    comment[COMMENT_SIZE] = 0;

    if (memcmp( comment, "FLIM\n", 5 ) != 0)
    {
        fprintf( stderr, "Not a valid flim file (bad signature)\n" );
        return false;
    }
    hdr.comment = comment + 5;

    //  Read checksum (2 bytes, big-endian)
    uint8_t checksum_bytes[CHECKSUM_SIZE];
    if (fread( checksum_bytes, 1, CHECKSUM_SIZE, f ) != CHECKSUM_SIZE)
    {
        fprintf( stderr, "Failed to read checksum\n" );
        return false;
    }
    const uint8_t *cp = checksum_bytes;
    hdr.checksum = read2( cp );

    //  Read header: version (2 bytes) + component count (2 bytes)
    uint8_t header_prefix[4];
    if (fread( header_prefix, 1, 4, f ) != 4)
    {
        fprintf( stderr, "Failed to read header\n" );
        return false;
    }
    const uint8_t *hp = header_prefix;
    hdr.version = read2( hp );
    uint16_t component_count = read2( hp );

    //  Read component directory: 10 bytes per entry (type:2 + offset:4 + size:4)
    hdr.components.resize( component_count );
    std::vector<uint8_t> dir_data( component_count * 10 );
    if (fread( dir_data.data(), 1, dir_data.size(), f ) != dir_data.size())
    {
        fprintf( stderr, "Failed to read component directory\n" );
        return false;
    }

    const uint8_t *dp = dir_data.data();
    for (int i = 0; i < component_count; i++)
    {
        hdr.components[i].type = read2( dp );
        hdr.components[i].offset = read4( dp );
        hdr.components[i].size = read4( dp );
    }

    hdr.data_start = COMMENT_SIZE + CHECKSUM_SIZE + 4 + component_count * 10;

    return true;
}

static void print_summary( const flim_header &hdr )
{
    printf( "Comment: %s\n", hdr.comment.c_str() );
    printf( "Checksum: 0x%04x\n", hdr.checksum );
    printf( "Version: %d\n", hdr.version );
    printf( "Components: %zu\n\n", hdr.components.size() );

    printf( "%-6s  %-10s  %-10s  %-10s\n", "Index", "Type", "Offset", "Size" );
    printf( "%-6s  %-10s  %-10s  %-10s\n", "-----", "----------", "----------", "----------" );
    for (size_t i = 0; i < hdr.components.size(); i++)
    {
        printf( "%-6zu  %-10s  %-10u  %-10u\n",
            i,
            component_type_name( hdr.components[i].type ),
            hdr.components[i].offset,
            hdr.components[i].size );
    }
}

static void print_info( FILE *f, const flim_header &hdr )
{
    for (auto &c : hdr.components)
    {
        if (c.type != component_info)
            continue;

        std::vector<uint8_t> data( c.size );
        fseek( f, hdr.data_start + c.offset, SEEK_SET );
        if (fread( data.data(), 1, data.size(), f ) != data.size())
        {
            fprintf( stderr, "Failed to read info component\n" );
            continue;
        }

        flim_info fi;
        fi.deserialize( data.data(), data.size() );

        printf( "\nInfo:\n" );
        printf( "  Dimensions: %zux%zu\n", fi.width_, fi.height_ );
        printf( "  Silent: %s\n", fi.silent_ ? "yes" : "no" );
        printf( "  Frames: %zu\n", fi.frame_count_ );
        printf( "  Total ticks: %zu\n", fi.total_ticks_ );
        printf( "  Byterate: %zu\n", fi.byterate_ );
    }
}

static void print_toc( FILE *f, const flim_header &hdr )
{
    for (auto &c : hdr.components)
    {
        if (c.type != component_toc)
            continue;

        std::vector<uint8_t> data( c.size );
        fseek( f, hdr.data_start + c.offset, SEEK_SET );
        if (fread( data.data(), 1, data.size(), f ) != data.size())
        {
            fprintf( stderr, "Failed to read toc component\n" );
            continue;
        }

        size_t frame_count = c.size / 2;
        const uint8_t *tp = data.data();

        printf( "\nTOC (%zu frames):\n", frame_count );
        printf( "  %-8s  %-10s  %-10s\n", "Frame", "Size", "Offset" );
        printf( "  %-8s  %-10s  %-10s\n", "--------", "----------", "----------" );

        size_t offset = 0;
        for (size_t frame = 0; frame < frame_count; frame++)
        {
            uint16_t frame_size = read2( tp );
            printf( "  %-8zu  %-10u  %-10zu\n", frame, frame_size, offset );
            offset += frame_size;
        }
    }
}

static bool extract_poster( FILE *f, const flim_header &hdr, const std::string &outpath )
{
    for (auto &c : hdr.components)
    {
        if (c.type != component_poster)
            continue;

        std::vector<uint8_t> data( c.size );
        fseek( f, hdr.data_start + c.offset, SEEK_SET );
        if (fread( data.data(), 1, data.size(), f ) != data.size())
        {
            fprintf( stderr, "Failed to read poster component\n" );
            return false;
        }

        //  Poster is a 1-bit packed bitmap (128x86, no header)
        size_t width = 128;
        size_t height = c.size / (width / 8);

        framebuffer fb( data, width, height, false );
        write_image( outpath.c_str(), fb.as_image() );

        printf( "\nPoster extracted to '%s' (%zux%zu)\n", outpath.c_str(), width, height );
        return true;
    }

    fprintf( stderr, "No poster component found\n" );
    return false;
}

static bool extract_initial( FILE *f, const flim_header &hdr, const std::string &outpath )
{
    for (auto &c : hdr.components)
    {
        if (c.type != component_initial)
            continue;

        std::vector<uint8_t> data( c.size );
        fseek( f, hdr.data_start + c.offset, SEEK_SET );
        if (fread( data.data(), 1, data.size(), f ) != data.size())
        {
            fprintf( stderr, "Failed to read initial component\n" );
            return false;
        }

        //  Initial frame has a 6-byte header: type(2) + width(2) + height(2)
        if (c.size < 6)
        {
            fprintf( stderr, "Initial component too small\n" );
            return false;
        }
        const uint8_t *p = data.data();
        /*uint16_t type =*/ read2( p );   //  0x00 = framebuffer
        uint16_t width = read2( p );
        uint16_t height = read2( p );

        std::vector<uint8_t> bitmap( p, p + (c.size - 6) );
        framebuffer fb( bitmap, width, height, false );
        write_image( outpath.c_str(), fb.as_image() );

        printf( "\nInitial frame extracted to '%s' (%ux%u)\n", outpath.c_str(), width, height );
        return true;
    }

    fprintf( stderr, "No initial frame component found\n" );
    return false;
}

int flimutil_main( int argc, char **argv )
{
    std::string path = *argv;
    argc--;
    argv++;

    //  Parse options
    bool show_info = false;
    bool show_toc = false;
    std::string poster_outpath;
    std::string initial_outpath;

    while (argc)
    {
        if (!strcmp( *argv, "--info" ))
            show_info = true;
        else if (!strcmp( *argv, "--toc" ))
            show_toc = true;
        else if (!strcmp( *argv, "--poster" ))
        {
            argc--;
            argv++;
            if (!argc)
            {
                fprintf( stderr, "--poster requires an output file path\n" );
                return EXIT_FAILURE;
            }
            poster_outpath = *argv;
        }
        else if (!strcmp( *argv, "--initial" ))
        {
            argc--;
            argv++;
            if (!argc)
            {
                fprintf( stderr, "--initial requires an output file path\n" );
                return EXIT_FAILURE;
            }
            initial_outpath = *argv;
        }
        else
        {
            fprintf( stderr, "Unknown option '%s'\n", *argv );
            return EXIT_FAILURE;
        }
        argc--;
        argv++;
    }

    FILE *f = fopen( path.c_str(), "rb" );
    if (!f)
    {
        fprintf( stderr, "Cannot open '%s'\n", path.c_str() );
        return EXIT_FAILURE;
    }

    flim_header hdr;
    if (!read_header( f, hdr ))
    {
        fclose( f );
        return EXIT_FAILURE;
    }

    print_summary( hdr );

    if (show_info)
        print_info( f, hdr );

    if (show_toc)
        print_toc( f, hdr );

    if (!poster_outpath.empty())
        extract_poster( f, hdr, poster_outpath );

    if (!initial_outpath.empty())
        extract_initial( f, hdr, initial_outpath );

    fclose( f );
    return EXIT_SUCCESS;
}
