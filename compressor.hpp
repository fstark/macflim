#ifndef COMPRESSOR_INCLUDED__
#define COMPRESSOR_INCLUDED__

#include <vector>
#include <bitset>
#include <iostream>
#include <algorithm>
#include <numeric>

#include "framebuffer.hpp"

/**
 * A compressor is a statefull object that generated the data needed to progress from an image to the next
 * It keeps track of what is currently on screen, and what should be, and find an efficient way
 * to generates the best update in a limited bandwidth
 */

template <typename T>
class compressor
{
    size_t W_;
    size_t H_;
    // static const size_t data_size = W_*H_/8/4;

    std::vector<T> current_data_;    //  The data present on screen (for optimisation purposes)
    std::vector<T> target_data_;     //  The data we are trying to converge to
    std::vector<size_t> delta_;                //  0: it is sync'ed

    size_t get_T_size() const { return W_*H_/8/sizeof(T); }

    static constexpr size_t get_T_bitcount() { return sizeof(T)*8; }

    size_t xcountbits( T v ) const
    {
        std::bitset<get_T_bitcount()> b{ v };
        return b.count();
    }

    size_t countbits( T v, size_t from, size_t to ) const
    {
        size_t res = 0;
        for (int i=from;i!=to;i++)
            res += !!(v&(1<<i));
        return res;
    }

    size_t distancebits( T v0, T v1, size_t from, size_t to ) const
    {
        auto c0 = countbits( v0, from, to );
        auto c1 = countbits( v1, from, to );
        return std::abs( (int)c0-(int)c1 );
    }

    size_t distance( T v0, T v1, size_t width ) const
    {
        size_t res = 0;
        for (size_t i=0;i!=get_T_bitcount();i+=width)
            res += distancebits( v0, v1, i, i+width );
        return res;
    }

    size_t distance( T v0, T v1 ) const
    {
        size_t v = xcountbits( v0^v1 );
        for (size_t i=1;i!=get_T_bitcount();i*=2)
            v += distance( v0, v1, i*2 );
        return v;
    }

    int frame = 0;

public:
    compressor( size_t W, size_t H) :  W_{W}, H_{H}, current_data_(get_T_size()), target_data_(get_T_size()), delta_(get_T_size())
    {
        std::fill( std::begin(current_data_), std::end(current_data_), 0xffffffff );
        std::fill( std::begin(target_data_), std::end(target_data_), 0xffffffff );
        std::fill( std::begin(delta_), std::end(delta_), 0x00000000 );
    }

    framebuffer get_current_framebuffer() const { return framebuffer(current_data_,W_,H_); }
 
    void set_current_image( const framebuffer &image )
    {
        current_data_ = image.raw_values<T>();
    }

   framebuffer get_target_framebuffer() const { return framebuffer(target_data_,W_,H_); }

    double quality() const
    {
        int b = 0;
        for (int i=0;i!=get_T_size();i++)
            b += xcountbits( current_data_[i]^target_data_[i] );
        return 1-b/(double)(W_*H_);
    }

        /// Sets the new target image
    void set_target_image( const framebuffer &image )
    {
        assert( image.W()==W_ && image.H()==H_ );
        auto new_data = image.raw_values<T>();
        for (int i=0;i!=get_T_size();i++)
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
                // printf( "DIFF %x %x = %ld\n", target_data_[i], current_data_[i], delta_[i] );
            }
        }
    }

    std::vector<run<T>> compress( size_t max_size )
    {
        size_t header_size = sizeof(T)==4?4:2;

        packzmap packmap{ get_T_size(), header_size, sizeof(T) };

        auto mx = *std::max_element( std::begin(delta_), std::end(delta_) );

        std::vector<std::vector<size_t>> deltas;
        deltas.resize( mx+1 );

        for (int i=0;i!=get_T_size();i++)
            if (delta_[i])
                deltas[delta_[i]].push_back( i );

        for (int i=deltas.size()-1;i!=0;i--)
            for (auto ix:deltas[i])
                if (packmap.set(ix)>=max_size)
                {
                    // std::clog << "Clearing at delta " << i << " size " << packmap.size() << " (index was:" << ix << ")\n";
                    packmap.clear(ix);
                    break;
                }

        return pack<T>(
            std::begin( target_data_ ),
            std::begin( packmap.mask() ),
            std::end( packmap.mask() ),
            max_size,
            W_/8/sizeof(T),
            H_
        );
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
    std::vector<u_int8_t> next_tick( size_t max_size )
    {
        frame++;

        // dump_histogram( "BEFORE" );

        auto runs = compress( max_size );

        for (auto &run:runs)
            if (run.offset>=get_T_size())
            {
                std::clog << "\n\n" << sizeof(T) << ": at " << run.offset << " " << run.data.size() << " data elements\n";
                assert( run.offset<get_T_size() );
            }


        // size_t size = 4;
        // for (auto &run:runs)
        // {
        //     size += sizeof(T);
        //     size += run.data.size()*sizeof(T);
        // }
        // std::clog << "APPROX COMPRESSED SIZE " << size << " / MAXSIZE " << max_size << "\n";

            //  Encode the runs
        std::vector<uint8_t> res;

        const int max_run_len = 127;
            //  Runs must not contain more than 256 bytes
        std::vector<run<T>> smaller;
        for (auto &run:runs)
        {
            // if (run.data.size()<255)
            //     smaller.push_back( run );
            // else
            //     std::clog << "SKIPPING!\n";
                //  #### : FIXME 32 words on screen
            auto rs = run.split( max_run_len, 64/sizeof(T) );

            for (auto &run2:rs)
                if (run2.offset>=get_T_size())
                {
                    std::clog << "\n" << sizeof(T) << ": at " << run.offset << " " << run.data.size() << " data elements split:\n";
                    std::clog << "" << sizeof(T) << ": at " << run2.offset << " " << run2.data.size() << " data elements\n";
                }

            smaller.insert( std::end(smaller), std::begin(rs), std::end(rs) );
        }

        for (auto &run:smaller)
        {
            if (run.offset>=get_T_size())
                std::clog << "\n\n" << sizeof(T) << ": at " << run.offset << " " << run.data.size() << " data elements\n";
            assert( run.offset<get_T_size() );
        }

            //  Sorts the runs
        std::sort( std::begin(smaller), std::end(smaller), [](auto &a, auto &b) {return a.offset < b.offset; } );

            //  Runs must be separated by at most 255 items
        std::vector<run<T>> closer;
        size_t offset = 0;
        for (auto &run:smaller)
        {
            while (run.offset-offset>255)
            {
                offset += 255;
                closer.push_back( { offset, {} } );
            }
            closer.push_back( run );
            offset = run.offset;
        }

        for (auto &run:closer)
            assert( run.offset<get_T_size() );

        // for (auto &run:closer)
        //     if (run.data.size()==0 && run.offset!=255)
        //         std::clog << "???" << "\n";

        //  Split long runs

            //  Compute the difference
        // size_t cur = 0;
        // for (auto &run:closer)
        // {
            // std::clog << run.offset-cur << "/" << run.data.size() << " ";

            // assert( run.data.size()<=max_run_len );
            // assert( run.offset-cur<=255 );

// << "[" << run.offset << "]"
            // cur = run.offset;
        // }
        // std::clog << "\n";


            //  Encode Z32
        if (sizeof(T)==4)
        {
            for (auto &run:runs)
            {
                uint32_t header = ((run.data.size()-1)<<16)+((run.offset+1)*sizeof(T));

                auto v = from_value( header );
                res.insert( std::end(res), std::begin(v), std::end(v) );
                auto vs = from_values( run.data );
                res.insert( std::end(res), std::begin(vs), std::end(vs) );
            }
            res.push_back( 0x00 );
            res.push_back( 0x00 );
            res.push_back( 0x00 );
            res.push_back( 0x00 );
        }

            //  Encode Z16
        if (sizeof(T)==2)
        {
            size_t current = 0;
            for (auto &run:closer)
            {
                uint16_t header = ((run.offset-current)<<8)+run.data.size();    //  oooooooo 0 sssssss
                current = run.offset;

                auto v = from_value( header );
                res.insert( std::end(res), std::begin(v), std::end(v) );
                auto vs = from_values( run.data );
                res.insert( std::end(res), std::begin(vs), std::end(vs) );
            }
            res.push_back( 0x00 );
            res.push_back( 0x00 );
        }

            //  #### Decompresses -- needs to be moved to the right object

        for (auto &run:closer)
        {
            
        // auto s = std::begin(res);
        // while (*s)
        // {
        //     uint32_t header = *s++;
        //     size_t offset = ((header&0xffff)-4);

            size_t offset = run.offset*sizeof(T);

            assert( run.offset<get_T_size() );

            size_t scr_x = offset%64;
            size_t scr_y = offset/64;

            scr_x /= sizeof(T);

            offset = scr_x * 342 + scr_y;

            for (auto &v:run.data)
            {
                assert( offset<get_T_size() );
                current_data_[offset] = v;
                delta_[offset] = 0;
                offset++;
            }

        //     int count = (header>>16)+1;

        //     while (count--)
        //     {
        //         current_data_[offset] = *s++;
        //         delta_[offset] = 0;
        //         offset++;
        //     }
        // };
        }

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
};

#endif
