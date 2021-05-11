//  ------------------------------------------------------------------
//  Compression utilities for B&W images
//  ------------------------------------------------------------------

#include <cstdint>
#include <stdlib.h>
#include <assert.h>
#include <memory.h>

int packbits( u_int8_t *out, const u_int8_t *buffer, int length );
int packzeroes( u_int8_t *out, const u_int8_t *const buffer, int length );
void unpackzeroesx( char *d, const char *s, size_t maxlen );

//  #### Note: u_int8_t is C not C++! (uint8_t)

//  ------------------------------------------------------------------
//  Pack the data by 32 bits blocks, compressing consecutive zeroes
//  ------------------------------------------------------------------
int packz32( u_int32_t *out, const u_int32_t *const buffer, int length );
void packz32_test();

#include <vector>
#include <array>

template <typename T>
void write1( T &out, uint32_t v )
{
    *out++ = v;
}

template <typename T>
void write2( T &out, uint32_t v )
{
    write1( out, v/256 );
    write1( out, v%256 );
}

template <typename T>
void write4( T &out, uint32_t v )
{
    write2( out, v/65536 );
    write2( out, v%65536 );
}

template <typename T>
void write( T &out, const std::vector<uint32_t> &vec )
{
    for (auto v:vec)
        write4( out, v );
}

template <typename T>
void write( T &out, const std::vector<uint8_t> &vec )
{
    for (auto v:vec)
        write1( out, v );
}

template <typename T, size_t N>
void write( T &out, const std::array<uint8_t,N> &arr )
{
    for (auto v:arr)
        write1( out, v );
}

class offset_t
{
    size_t offset_ = 0;

public:
    size_t linear() { return offset_; }
    bool increment()
    {
        offset_ += 64;
        if (offset_>21888)
        {
            offset_ -= 21888-4;
            return true;
        }
        return false;
    }
};


//  ------------------------------------------------------------------
//  Conditionally pack data
//  ------------------------------------------------------------------
//  Packs the buffer where pack is true, returns the packed data
// std::vector<uint32_t> packz32opt( const std::vector<uint32_t> buffer, const std::vector<bool> pack );
template <typename T0, typename T1>
//data: iterator to start of data
//pack_begin: iterator begin of pack instruction (boolean array => true means keep, false means skip)
//pack_end: iterator end of pack instruction.
std::vector<uint32_t> packz32opt( T0 data, T1 pack_begin, T1 pack_end, size_t max_pack )
{
    std::vector<uint32_t> output_buffer;
    auto out = std::back_inserter(output_buffer);

    offset_t offset;
    uint32_t linear_offset;

    while (pack_begin<pack_end)
    {
        //  We look for the next non-zero
        while (pack_begin<pack_end && !*pack_begin)
        {
            data++;
            pack_begin++;
            offset.increment();
        }
        linear_offset = offset.linear();

        //  We look for the next zero
        size_t non_zero_count = 0;
        while (pack_begin<pack_end && *pack_begin)
        {
            non_zero_count++;
            pack_begin++;

            if (offset.increment())
                break;

            if (1+non_zero_count+output_buffer.size()>=max_pack)
                break;
        }

        if (non_zero_count==0)      //  Don't skip at the end if nothing needs to be copied
            break;

        *out++ = ((non_zero_count-1)<<16)+(linear_offset+4);

        while (non_zero_count--)
            *out++ = *data++;

            //  Abort if more than max_pack bytes
        if (output_buffer.size()>=max_pack)
            break;
    }

    *out++ = 0;

    return output_buffer;
}

template <typename T, typename U>
std::vector<uint32_t> packz32opt( T data, U pack, size_t max_pack = 21888 ) { return packz32opt( std::begin(data), std::begin(pack), std::end(pack), max_pack ); }


void packz32opt_test();

#include <array>

class packzmap
{
private:
    std::vector<bool> mask_;
    size_t N;

    size_t size_;

        //  Fills hole if free or better
    void auto_fill( size_t n )
    {
        assert( n<N );
        // if (n>N)
            // exit(1);
        if (!mask_[n])
        {
            if (n>0 && n<N-1 && mask_[n-1] && mask_[n+1])
                set( n );
            if (n==0 && mask_[1])
                set( 0 );
            if (n==N-1 && mask_[N-2])
                set( N-1 );
        }
    }

public:
    packzmap( size_t map_size ) : mask_( map_size ), N{ map_size }
    {
        std::fill( std::begin(mask_), std::end(mask_), false );
        size_ = 1;  //  End marker
    }

    auto begin() const { return std::begin(mask_); }
    auto end() const { return std::end(mask_); }
    size_t size() const { return size_; }

    size_t set( size_t n )
    {
        assert( n<N );

        if (mask_[n])
            return size();
        mask_[n] = true;
        size_ += 2;
            //  Collapses with previous
        if (n>0 && mask_[n-1])
            size_--;
            //  Collapses with next
        if (n<N-1 && mask_[n+1])
            size_--;

        //  Auto-optimize
        if (n>0)
            auto_fill( n-1 );
        if (n+1<N)
            auto_fill( n+1 );

        return size();
    }

    size_t clear( size_t n )
    {
        assert( n<N );

        if (!mask_[n])
            return size();
        mask_[n] = false;
            //  by default, removes one data, but adds a header, so no change
        size_ += 0;
            //  Collapses with next if previous empty (don't add header)
        if (n>1 && !mask_[n-1])
            size_--;
            //  Collapses with previous if next empty (don't add header)
        if (n<N-1 && !mask_[n+1])
            size_--;
            //  If both, we removes both header and data

        return size();
    }
};
