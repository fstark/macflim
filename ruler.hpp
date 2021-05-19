#ifndef RULER_INCLUDED__
#define RULER_INCLUDED__

#include <bitset>
#include <iostream>
#include <numeric>
#include <cstdint>
#include <bit>
#include <limits>
#include <cassert>

template <typename T>
class ruler
{
    public:
        virtual size_t distance( T x, T y ) const = 0;
        virtual ~ruler() {}
};

class uint8_ruler : public ruler<uint8_t>
{
    size_t distance_[256][256];
    bool known_[256][256];

    bool set_distance( int n0, int n1, size_t v )
    {
        if (v>=distance_[n0][n1])
            return false;

        distance_[n0][n1] = distance_[n1][n0] = v;
        known_[n0][n1] = known_[n1][n0] = true;

        return true;
    }

    void set_distance( int n0, std::bitset<8> n1, size_t v ) { set_distance( n0, n1.to_ulong(), v ); }
    void set_distance( std::bitset<8> n0, int n1, size_t v ) { set_distance( n1, n0, v ); }
    void set_distance( std::bitset<8> n0, std::bitset<8> n1, size_t v ) { set_distance( n0.to_ulong(), n1.to_ulong(), v ); }

    bool known( int x, int y ) { return known_[x][y]; }

    void complete()
    {
        bool work_needed = true;

        while (work_needed)
        {
            work_needed = false;
            // std::clog << "Pass....\n";
            for (int x=0;x!=256;x++)
                for (int y=0;y!=256;y++)
                {   for (int z=0;z!=256;z++)
                        if (known(x,z) && known(y,z))
                            work_needed |= set_distance( x, y, distance( x, z )+distance( z, y ) );
                    work_needed |= !known( x,y );
                }
        } ;      
    }

public:
    uint8_ruler()
    {
        for (int x=0;x!=256;x++)
            for (int y=0;y!=256;y++)
            {
                distance_[x][y] = std::numeric_limits<size_t>::max();
                known_[x][y] = false;
            }

        for (int n=0;n!=256;n++)
        {
            set_distance( n, n, 0 );

            std::bitset<8> bin(n);

            for (int b=0;b!=8;b++)
            {
                set_distance( n, bin.flip(b), 2 );
                bin.flip(b);
            }
        }

        for (int n=0;n!=256;n++)
            for (int b=0;b!=7;b++)
            {
                std::bitset<8> bin(n);
                if (bin[b]!=bin[b+1])
                {
                    bin.flip(b);
                    bin.flip(b+1);
                    set_distance( n, bin, 1 );
                }
            }

        complete();
    }

    virtual size_t distance( uint8_t x, uint8_t y ) const { return distance_[x][y]; }

    static const uint8_ruler ruler;
};

class uint16_ruler : public ruler<uint16_t>
{
    const uint8_ruler &byte_ruler_ = uint8_ruler::ruler;

public:
    virtual size_t distance( uint16_t x, uint16_t y ) const
    {
        return byte_ruler_.distance(x>>8,y>>8)
             + byte_ruler_.distance(x&0xff,y&0xff);
    }

    static const uint16_ruler ruler;
};

class uint32_ruler : public ruler<uint32_t>
{
    const uint8_ruler &byte_ruler_ = uint8_ruler::ruler;

public:
    virtual size_t distance( uint32_t x, uint32_t y ) const
    {
        return byte_ruler_.distance(x>>24,y>>24)
             + byte_ruler_.distance((x>>16)&0xff,(y>>16)&0xff)
             + byte_ruler_.distance((x>>8)&0xff,(y>>8)&0xff)
             + byte_ruler_.distance(x&0xff,y&0xff);
    }

    static const uint32_ruler ruler;
};


template <typename T>
class bit_ruler : public ruler<T>
{
    constexpr size_t get_T_bitcount() const
    {
        return sizeof(T)*8;
    }

    constexpr size_t countbits( T v, size_t from, size_t to ) const
    {
        size_t res = 0;
        for (int i=from;i!=to;i++)
            res += !!(v&(1<<i));
        return res;
    }

    constexpr size_t distance( T v0, T v1, size_t width ) const
    {
        size_t res = 0;
        for (size_t i=0;i!=get_T_bitcount();i+=width)
        {
            auto c0 = countbits( v0, i, i+width );
            auto c1 = countbits( v1, i, i+width );
            res += std::abs( (int)c0-(int)c1 );
        }
        return res;
    }

public:
    virtual size_t distance( T v0, T v1 ) const
    {
        size_t v = std::popcount( (T)(v0^v1) );         //  beware uint16_t ^ uint16_t is an int
        for (size_t i=1;i!=get_T_bitcount();i*=2)
            v += distance( v0, v1, i*2 );
        return v;
    }
};

#endif
