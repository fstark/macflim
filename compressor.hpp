#ifndef COMPRESSOR_INCLUDED__
#define COMPRESSOR_INCLUDED__

#include <vector>
#include <bitset>
#include <iostream>
#include <algorithm>

#include "framebuffer.hpp"

/**
 * A compressor is a statefull object that generated the data needed to progress from an image to the next
 * It keeps track of what is currently on screen, and what should be, and find an efficient way
 * to generates the best update in a limited bandwidth
 */

class compressor
{
    size_t W_;
    size_t H_;
    // static const size_t data_size = W_*H_/8/4;

    std::vector<uint32_t> current_data_;    //  The data present on screen (for optimisation purposes)
    std::vector<uint32_t> target_data_;     //  The data we are trying to converge to
    std::vector<size_t> delta_;                //  0: it is sync'ed

    size_t  get_uint32_size() const { return W_*H_/8/4; }

    size_t xcountbits( uint32_t v ) const
    {
        std::bitset<32> b{ v };
        return b.count();
    }

    size_t countbits( uint32_t v, size_t from, size_t to ) const
    {
        size_t res = 0;
        for (int i=from;i!=to;i++)
            res += !!(v&(1<<i));
        return res;
    }

    size_t distancebits( uint32_t v0, uint32_t v1, size_t from, size_t to ) const
    {
        auto c0 = countbits( v0, from, to );
        auto c1 = countbits( v1, from, to );
        return std::abs( (int)c0-(int)c1 );
    }

    size_t distance( uint32_t v0, uint32_t v1, size_t width ) const
    {
        size_t res = 0;
        for (size_t i=0;i!=32;i+=width)
            res += distancebits( v0, v1, i, i+width );
        return res;
    }

    size_t distance( uint32_t v0, u_int32_t v1 ) const
    {
        return xcountbits( v0^v1 ) + distance( v0, v1, 2 ) + distance( v0, v1, 4 ) + distance( v0, v1, 8 ) + distance( v0, v1, 16 ) + distance( v0, v1, 32 );
        // return xcountbits( v0^v1 );
    }

    int frame = 0;

public:
    compressor( size_t W, size_t H) : current_data_(W*H/8/4), target_data_(W*H/8/4), delta_(W*H/8/4), W_{W}, H_{H}
    {
        std::fill( std::begin(current_data_), std::end(current_data_), 0xffffffff );
        std::fill( std::begin(target_data_), std::end(target_data_), 0xffffffff );
        std::fill( std::begin(delta_), std::end(delta_), 0x00000000 );
    }

    framebuffer get_current_framebuffer() const { return framebuffer(current_data_,W_,H_); }
    framebuffer get_target_framebuffer() const { return framebuffer(target_data_,W_,H_); }

    double quality() const
    {
        int b = 0;
        for (int i=0;i!=get_uint32_size();i++)
            b += xcountbits( current_data_[i]^target_data_[i] );
        return 1-b/(double)(W_*H_);
    }

    std::vector<uint32_t> compress( size_t max_size )
    {
        packzmap packmap{ get_uint32_size() };

        auto mx = *std::max_element( std::begin(delta_), std::end(delta_) );

        std::vector<std::vector<size_t>> deltas;
        deltas.resize( mx+1 );

        for (int i=0;i!=get_uint32_size();i++)
            if (delta_[i])
                deltas[delta_[i]].push_back( i );

        for (int i=deltas.size()-1;i!=0;i--)
            for (auto ix:deltas[i])
                if (packmap.set(ix)>=max_size)
                {
                    packmap.clear(ix);
                    break;
                }

        return packz32opt( target_data_, packmap.mask() );
    }

    void dump_histogram( const std::string msg )
    {
        auto mx = *std::max_element( std::begin(delta_), std::end(delta_) );
        std::clog << msg << " ";
        for (int i=0;i<=mx;i++)
            fprintf( stderr, "%3d:%5ld ", i, std::count( std::begin(delta_), std::end(delta_), i ) );
            // std::clog << i << ":" << std::count( std::begin(delta_), std::end(delta_), i ) <<"  ";
        std::clog << "\n";
    }

        /// Progress toward the target image, using less than max_size bytes
    std::vector<uint32_t> next_tick( size_t max_size )
    {
        frame++;

        // dump_histogram( "BEFORE" );

        auto res = compress( max_size );

        auto s = std::begin(res);

        while (*s)
        {
            uint32_t header = *s++;
            uint32_t offset = ((header&0xffff)-4);

            uint32_t scr_x = offset%64;
            uint32_t scr_y = offset/64;

            scr_x /= 4;

            offset = scr_x * 342 + scr_y;

            int count = (header>>16)+1;

            while (count--)
            {
                current_data_[offset] = *s++;
                delta_[offset] = 0;
                offset++;
            }
        };

        // dump_histogram( "AFTER " );

        //  logs current image
        {
            static int img = 1;
            char buffer[1024];
            sprintf( buffer, "out-%06d.pgm", img );
            framebuffer fb{current_data_, W_, H_};
            auto logimg = fb.as_image();
            write_image( buffer, logimg );

            img++;
        }
        return res;
    }

        /// Sets the new target image
    void set_target_image( const framebuffer &image )
    {
        assert( image.W()==W_ && image.H()==H_ );
        auto new_data = image.raw_values<uint32_t>();
        for (int i=0;i!=get_uint32_size();i++)
        {
            target_data_[i] = new_data[i];
            if (current_data_[i]==target_data_[i])
                    //  Data is identical, we don't care about updating this
                delta_[i] = 0;
            else
            {
                    //  Let's increase the importance of updating this
                // delta_[i] += countbits( target_data_[i] ^ current_data_[i] );
                delta_[i] = distance( target_data_[i], current_data_[i] );
                // printf( "DIFF %x %x = %d\n", target_data_[i], current_data_[i], diff_[i] );
            }
        }
    }
};

#endif
