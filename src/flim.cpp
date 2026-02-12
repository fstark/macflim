#include "flim.hpp"

#include <cstdio>
#include <cstring>

//  --- Fletcher checksum ---

void fletcher( long &checksum, const std::vector<uint8_t> &data )
{
    assert( (data.size()%2)==0 );
    for (size_t i=0;i!=data.size();i+=2)
    {
        checksum += ((int)(data[i]))*256+data[i+1];
        checksum %= 65535;
    }
}

void fletcher( long &checksum, uint16_t data )
{
    checksum += data;
    checksum %= 65535;
}

//  --- flim constructor ---

flim::flim( const std::string &comment ) : comment_{ comment }
{
    if (comment_.size() < COMMENT_SIZE)
        comment_.resize( COMMENT_SIZE, 0x00 );
    else
        comment_ = comment_.substr( 0, COMMENT_SIZE );

    comment_[0] = 'F';
    comment_[1] = 'L';
    comment_[2] = 'I';
    comment_[3] = 'M';
    comment_[4] = '\n';
}

//  --- flim::read ---

bool flim::read( FILE *f )
{
    //  Read comment block
    char comment_buf[COMMENT_SIZE + 1];
    if (fread(comment_buf, 1, COMMENT_SIZE, f) != COMMENT_SIZE)
    {
        fprintf(stderr, "Failed to read comment block\n");
        return false;
    }
    comment_buf[COMMENT_SIZE] = 0;

    if (memcmp(comment_buf, "FLIM\n", 5) != 0)
    {
        fprintf(stderr, "Not a valid flim file (bad signature)\n");
        return false;
    }
    comment_ = comment_buf;

    //  Read checksum (2 bytes, big-endian)
    uint8_t checksum_bytes[CHECKSUM_SIZE];
    if (fread(checksum_bytes, 1, CHECKSUM_SIZE, f) != CHECKSUM_SIZE)
    {
        fprintf(stderr, "Failed to read checksum\n");
        return false;
    }
    const uint8_t *cp = checksum_bytes;
    /*uint16_t stored_checksum =*/ read2(cp);

    //  Read header: version (2 bytes) + component count (2 bytes)
    uint8_t header_prefix[4];
    if (fread(header_prefix, 1, 4, f) != 4)
    {
        fprintf(stderr, "Failed to read header\n");
        return false;
    }
    const uint8_t *hp = header_prefix;
    version_ = read2(hp);
    uint16_t component_count = read2(hp);

    //  Read component directory: 10 bytes per entry (type:2 + offset:4 + size:4)
    components_.resize(component_count);
    std::vector<uint8_t> dir_data(component_count * 10);
    if (fread(dir_data.data(), 1, dir_data.size(), f) != dir_data.size())
    {
        fprintf(stderr, "Failed to read component directory\n");
        return false;
    }

    const uint8_t *dp = dir_data.data();
    for (int i = 0; i < component_count; i++)
    {
        components_[i].type = read2(dp);
        components_[i].offset = read4(dp);
        components_[i].size = read4(dp);
    }

    //  Read all component data blobs
    long data_start = COMMENT_SIZE + CHECKSUM_SIZE + 4 + component_count * 10;
    blobs_.resize(component_count);
    for (int i = 0; i < component_count; i++)
    {
        blobs_[i].resize(components_[i].size);
        fseek(f, data_start + components_[i].offset, SEEK_SET);
        if (fread(blobs_[i].data(), 1, blobs_[i].size(), f) != blobs_[i].size())
        {
            fprintf(stderr, "Failed to read component %d data\n", i);
            return false;
        }
    }

    return true;
}

//  --- flim::serialize_header ---

std::vector<uint8_t> flim::serialize_header() const
{
    std::vector<uint8_t> header;
    auto o = std::back_inserter(header);

    write2( o, version_ );
    write2( o, components_.size() );

    size_t offset = 0;
    for (size_t i = 0; i < components_.size(); i++)
    {
        write2( o, components_[i].type );
        write4( o, offset );
        write4( o, blobs_[i].size() );
        offset += blobs_[i].size();
    }

    return header;
}

//  --- flim::compute_checksum ---

uint16_t flim::compute_checksum() const
{
    long checksum = 0;
    ::fletcher( checksum, serialize_header() );

    printf( "HEADER: %ld\n", checksum );
    for (auto &blob : blobs_)
    {
        ::fletcher( checksum, blob );
        printf( "-> : %ld\n", checksum );
    }

    return checksum;
}

//  --- flim::write ---

void flim::write_u16( FILE *f, uint16_t v )
{
    uint8_t b = v / 256;
    ::fwrite( &b, 1, 1, f );
    b = v % 256;
    ::fwrite( &b, 1, 1, f );
}

void flim::write_bytes( FILE *f, const std::vector<uint8_t> &v )
{
    ::fwrite( v.data(), v.size(), 1, f );
}

void flim::write( FILE *f ) const
{
    ::fwrite( comment_.c_str(), COMMENT_SIZE, 1, f );
    write_u16( f, compute_checksum() );
    write_bytes( f, serialize_header() );
    for (auto &blob : blobs_)
        write_bytes( f, blob );
}

//  --- Building helpers ---

void flim::add_component( eComponentType type, const std::vector<uint8_t> &data )
{
    components_.push_back( { type, 0, static_cast<uint32_t>(data.size()) } );
    blobs_.push_back( data );
}

void flim::add( const flim_info &fi )
{
    std::vector<uint8_t> data;
    fi.serialize( data );
    add_component( component_info, data );
}

void flim::add( const std::vector<frame> &frames )
{
    std::vector<uint8_t> movie_data;
    std::vector<uint8_t> toc_data;

    auto ot = std::back_inserter(toc_data);

    for (auto &f : frames)
    {
        size_t prev_size = movie_data.size();
        f.serialize( movie_data );
        write2( ot, movie_data.size() - prev_size );
    }

    add_component( component_movie, movie_data );
    add_component( component_toc, toc_data );
}

void flim::add_framebuffer( eComponentType type, const bitmap &fb )
{
    std::vector<uint8_t> data;
    auto bi = std::back_inserter(data);
    write2( bi, 0x00 );         //  Framebuffer
    write2( bi, fb.W() );
    write2( bi, fb.H() );
    auto img_data = fb.raw_values_natural<uint8_t>();
    data.insert( std::end(data), std::begin(img_data), std::end(img_data) );
    add_component( type, data );
}

void flim::add_poster( const bitmap &fb )
{
    std::vector<uint8_t> data = fb.raw_values_natural<uint8_t>();
    add_component( component_poster, data );
}

void flim::add_initial( const bitmap &fb )
{
    add_framebuffer( component_initial, fb );
}
