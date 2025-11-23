#pragma once

//  ------------------------------------------------------------------
//  Compression utilities for B&W images
//  ------------------------------------------------------------------

#include <cstdint>
#include <stdlib.h>
#include <assert.h>
#include <memory.h>

int packbits( uint8_t *out, const uint8_t *buffer, int length );
int packzeroes( uint8_t *out, const uint8_t *const buffer, int length );
void unpackzeroesx( char *d, const char *s, size_t maxlen );

//  #### Note: Using uint8_t (standard C++) instead of uint8_t (BSD)

//  ------------------------------------------------------------------
//  Pack the data by 32 bits blocks, compressing consecutive zeroes
//  ------------------------------------------------------------------
int packz32( uint32_t *out, const uint32_t *const buffer, int length );
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

//  Maps a linear offset to the vertical data to the horizonal offsets
class offset_t
{
    size_t width_;
    size_t height_;

    size_t offset_ = 0;

public:
    offset_t( size_t width, size_t height ) : width_{ width }, height_{ height } {}

    size_t linear() { return offset_; }
    bool increment()
    {
        offset_ += width_;
        if (offset_>=width_*height_)
        {
            offset_ -= width_*height_-1;
            return true;
        }
        return false;
    }
};


template <typename T>
struct run
{
    size_t offset;          //  Offset expressed in Ts
    std::vector<T> data;    //  Data to be written at offset

    bool operator==( const run &rhs ) const
    {
        return offset==rhs.offset && data==rhs.data;
    }

        //  Split the run in a series of new runs, none larger than max_size
    std::vector<run> split( size_t max_size, size_t strideT ) const
    {
        std::vector<run> res;

        run r;
        r.offset = offset;

        auto p = std::begin(data);
        while (p!=std::end(data))
        {
            r.data.push_back( *p++ );
            if (r.data.size()==max_size && p!=std::end(data))
            {
                res.push_back( r );
                r.offset += max_size*strideT;
                r.data.resize(0);
            }
        }

        res.push_back( r );

        return res;
    }
};

#include <iostream>

const size_t kHeaderSize = 2;

template <typename T>
inline std::vector<run<T>> pack(
    typename std::vector<T>::const_iterator data,
    std::vector<bool>::const_iterator pack_begin,   //  Given in real screen order, probably a mistake
    std::vector<bool>::const_iterator pack_end,     //  (offset_t does the automatic conversion, so we scan in vertical order)
    size_t max_pack_bytes,
    size_t width,
    size_t height
        )
{
    std::vector<run<T>> output_buffer;
    offset_t offset{ width, height };

    size_t total_bytes = kHeaderSize; //  end-marker

    while (pack_begin<pack_end)
    {
        run<T> run;

        //  We look for the next non-zero
        while (pack_begin<pack_end && !*pack_begin)
        {
            data++;
            ++pack_begin;
            offset.increment();
        }
        run.offset = offset.linear();

        //  We look for the next zero
        size_t non_zero_count = 0;
        while (pack_begin<pack_end && *pack_begin)
        {
            non_zero_count++;
            ++pack_begin;

            if (offset.increment())
                break;

            if (total_bytes+kHeaderSize+sizeof(T)*non_zero_count>=max_pack_bytes)
                break;
        }

        if (non_zero_count==0)      //  Don't skip at the end if nothing needs to be copied
            break;

        total_bytes += kHeaderSize + sizeof(T)*non_zero_count;

        while (non_zero_count--)
            run.data.push_back( *data++ );

        output_buffer.push_back( run );

            //  Abort if more than max_pack bytes
        if (total_bytes>=max_pack_bytes)
            break;
    }

    //  Count the number of unpacked items (quality of the packmap)

#if 0
    size_t err_count = 0;
    while (pack_begin!=pack_end)
        err_count += *pack_begin++;

    std::clog << "UNPACKED = " << err_count << "\n";
#endif

    return output_buffer;
}

#if 0
//  ------------------------------------------------------------------
//  Conditionally pack data
//  ------------------------------------------------------------------
//  Packs the buffer where pack is true, returns the packed data
// std::vector<uint32_t> packz32opt( const std::vector<uint32_t> buffer, const std::vector<bool> pack );
//data: iterator to start of data
//pack_begin: iterator begin of pack instruction (boolean array => true means keep, false means skip)
//pack_end: iterator end of pack instruction.
inline std::vector<uint32_t> packz32opt( std::vector<uint32_t>::const_iterator data, std::vector<bool>::const_iterator pack_begin, std::vector<bool>::const_iterator pack_end, size_t max_pack )
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
            ++pack_begin;
            offset.increment();
        }
        linear_offset = offset.linear();

        //  We look for the next zero
        size_t non_zero_count = 0;
        while (pack_begin<pack_end && *pack_begin)
        {
            non_zero_count++;
            ++pack_begin;

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
#endif

#include <array>

//  Computing the size
//  Each run have a fixed overhead of 4 bytes
class packzmap
{
private:
    std::vector<bool> mask_;
    size_t N;

    size_t byte_size_;

    size_t header_cost_;
    size_t elem_cost_;

    size_t dbg_calc_size() const
    {
        size_t res = header_cost_;
        bool state = false;
        for (auto b:mask_)
        {
            if (b && state==false)
            {
                res += header_cost_;
                state = true;
            }

            if (b)
                res += elem_cost_;

            if (!b)
            {
                state = false;
            }
        }

        return res;
    }

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
    packzmap( size_t map_size, size_t header_cost, size_t elem_cost ) : mask_( map_size ), N{ map_size }, header_cost_{header_cost}, elem_cost_{elem_cost}
    {
        std::fill( std::begin(mask_), std::end(mask_), false );
        byte_size_ = header_cost_;  //  End marker
    }

    const std::vector<bool> &mask() const { return mask_; }

    size_t size() const
    {
        if (byte_size_>header_cost_+N*elem_cost_)
        {
            std::cerr << "Byte size  : " << byte_size_ << "\n";
            std::cerr << "Header Cost: " << header_cost_ << "\n";
            std::cerr << "Elem Cost  : " << elem_cost_ << "\n";
            std::cerr << "N          : " << N << "\n";
        }
        assert( byte_size_<=header_cost_*2+N*elem_cost_ );
        return byte_size_;
    }

    size_t set( size_t n )
    {
// std::clog << n << ":" << byte_size_ << "/" << dbg_calc_size() << " ";

        assert( n<N );

        if (mask_[n])
            return size();
        mask_[n] = true;
        byte_size_ += header_cost_ + elem_cost_;

            //  Collapses with previous
        if (n>0 && mask_[n-1])
            byte_size_-= header_cost_;

            //  Collapses with next
        if (n<N-1 && mask_[n+1])
            byte_size_-= header_cost_;

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

// if (n==0 && mask_[1])
//     std::clog << "WEIRD CLEAR @0\n";
// if (n==N-1 && mask_[N-2])
//     std::clog << "WEIRD CLEAR @N-1\n";

        mask_[n] = false;
            //  by default, removes one data, but adds a header
        byte_size_ += header_cost_;
        byte_size_ -= elem_cost_;

            //  Collapses with next if previous empty (don't add header)
        if (n>1 && !mask_[n-1])
            byte_size_ -= header_cost_;

            //  Collapses with previous if next empty (don't add header)
        if (n<N-1 && !mask_[n+1])
            byte_size_ -= header_cost_;
            //  If both, we removes both header and data

        return size();
    }

    bool empty_border( size_t n )
    {
        if (mask_[n])
            return false;
        if (n>0 && mask_[n-1])
            return true;
        if (n<N-1 && mask_[n+1])
            return true;

        return false;
    }
};


// inline std::vector<uint32_t> packz32opt( const std::vector<uint32_t> &data, const std::vector<bool> &pack, size_t max_pack = 21888 ) { return packz32opt( std::begin(data), std::begin(pack), std::end(pack), max_pack ); }


void packz32opt_test();
