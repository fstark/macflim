#ifndef COMPRESSOR_INCLUDED__
#define COMPRESSOR_INCLUDED__

#include <vector>
#include <bitset>
#include <iostream>
#include <algorithm>
#include <numeric>
#include <cstdint>
#include <bit>
#include <limits>

#include "framebuffer.hpp"
#include "ruler.hpp"

inline bool bool_from( const std::string &v )
{
    if (v=="true")
        return true;
    return false;
}

/**
 * Encapsulate a way to compress a single frame transition
 */
class compressor
{
protected:
    mutable bool verbose_ = false;

    static size_t size_t_from( const std::string &v )
    {
        return atoi(v.c_str());
    }

public:
    virtual ~compressor() {}
    virtual std::vector<uint8_t> compress( framebuffer &current, const framebuffer &target, /* weigths, */ size_t budget ) const = 0;

    virtual bool set_parameter( const std::string parameter, const std::string value )
    {
        if (parameter=="verbose")
            verbose_ = bool_from( value );
        return false;
    }

    virtual std::string name() const = 0;

    virtual std::string description() const
    {
        return name();
    }
};

class null_compressor : public compressor
{
    virtual std::string name() const { return "null"; };
public:
    virtual std::vector<uint8_t> compress( framebuffer &current, const framebuffer &target, /* weigths, */ size_t budget ) const
    {
        return {};
    }
};

class invert_compressor : public compressor
{
    virtual std::string name() const { return "invert"; };
public:
    virtual std::vector<uint8_t> compress( framebuffer &current, const framebuffer &target, /* weigths, */ size_t budget ) const
    {
        current = current.inverted();
        return {};
    }
};

class copy_line_compressor : public compressor
{
    virtual std::string name() const { return "lines"; };

    virtual bool set_parameter( const std::string parameter, const std::string value )
    {
        return compressor::set_parameter( parameter, value );
    }

    virtual std::vector<uint8_t> compress( framebuffer &current, const framebuffer &target, /* weigths, */ size_t budget ) const
    {
        framebuffer result{current};

        size_t q = 0;

        size_t line_start = 0;
        size_t line_count = 0;

        size_t target_count = budget / 64;  //  est. 64 bytes per line

// std::clog << "Lines: " << budget << " bytes " << target_count << " lines \n";

        for (size_t i=0;i<342;i+=target_count)
        {
            framebuffer fb = current;
            size_t lc = std::min( target_count, 342-i );
            fb.copy_lines_from( target, i, lc );
            auto res = fb.count_differences( current );
            if (res>q)
            {
                q = res;
                result = fb;
                // std::clog << "[" << q << "]";
                line_start = i;
                line_count = lc;
            }
        }
        current = result;

        std::vector<uint8_t> data;
        auto out = std::back_inserter( data );

        write2( out, line_count*64 );
        write2( out, line_start*64 );

        target.extract( out, 0, line_start, line_count*64 );

        return data;
    }
};

/**
 * Compresses an image using vertical strips of various width
 */
template <typename T>
class vertical_compressor : public compressor
{
    virtual std::string name() const { char buffer[1024]; sprintf( buffer, "z%lu", sizeof(T)*8 ); return buffer; }

    size_t W_;
    size_t H_;

    const ruler<T> &ruler_;

    size_t get_T_width() const { return W_/8/sizeof(T); }
    size_t get_T_size() const { return get_T_width()*H_; }

    std::vector<run<T>> compress( size_t max_size, const std::vector<T> &target_data_, const std::vector<size_t> &delta_ ) const
    {
        size_t header_size = sizeof(T)==4?4:2;

        packzmap packmap{ get_T_size(), header_size, sizeof(T) };

        auto mx = *std::max_element( std::begin(delta_), std::end(delta_) );

        std::vector<std::vector<size_t>> deltas;
        deltas.resize( mx+1 );

        for (int i=0;i!=get_T_size();i++)
            if (delta_[i])
                deltas[delta_[i]].push_back( i );

        bool done = false;

        for (int i=deltas.size()-1;i!=0;i--)
        {
            for (auto ix:deltas[i])
                if (packmap.set(ix)>=max_size)
                {
                    // std::clog << "Clearing at delta " << i << " size " << packmap.size() << " (index was:" << ix << ")\n";
                    packmap.clear(ix);
                    done = true;
                    break;
                }

            if (done)
                break;

            //  We add the borders of the packmap if not "expensive"
            for (int ix=0;ix!=get_T_size();ix++)
                if (((ix%H_)!=0) && ((ix%H_)!=H_-1) && packmap.empty_border(ix))
                    if (delta_[ix]*2>=i)
                        if (packmap.set(ix)>=max_size)
                        {
                            packmap.clear(ix);
                            done = true;
                            break;
                        }

            if (done)
                break;
        }

        auto res = pack<T>(
            std::begin( target_data_ ),
            std::begin( packmap.mask() ),
            std::end( packmap.mask() ),
            max_size,
            W_/8/sizeof(T),
            H_
        );

        if (verbose_)
            std::clog << "=> z" << sizeof(T)*8 << " Generated " << res.size() << " runs\n";

        return res;
    }

public:
    vertical_compressor( size_t W, size_t H, const ruler<T> &ruler ) :  W_{W}, H_{H}, ruler_{ruler}
    {
    }

size_t vertical_from_horizontal( size_t h ) const
{
    size_t offset = h*sizeof(T);

    assert( h<get_T_size() );

    size_t scr_x = offset%64;
    size_t scr_y = offset/64;

    scr_x /= sizeof(T);

    offset = scr_x * 342 + scr_y;

    return offset;
}


    virtual std::vector<uint8_t> compress( framebuffer &current, const framebuffer &target, /* weigths, */ size_t budget ) const
    {
// std::cerr << "BUDGET:" << budget << "\n";

            //  transient
        auto current_data_ = current.raw_values<T>();    //  The data present on screen (for optimisation purposes) (vertical)
        auto target_data_ = target.raw_values<T>();      //  The data we are trying to converge to
        std::vector<size_t> delta_(get_T_size());        //  0: it is sync'ed

        for (int i=0;i!=get_T_size();i++)
        {
            if (current_data_[i]==target_data_[i])
                    //  Data is identical, we don't care about updating this
                delta_[i] = 0;
            else
            {
                    //  Let's increase the importance of updating this
                // delta_[i] += countbits( target_data_[i] ^ current_data_[i] );
                delta_[i] = ruler_.distance( target_data_[i], current_data_[i] );
            }

        }
            //  Display delta map in correct order
        if (verbose_)
        {
            std::clog << "DELTA LIST OF " << get_T_size() << " elements: [\n";
            for (size_t y=0;y!=H_;y++)
            {
                fprintf( stderr, "    " );
                for (size_t x=0;x!=get_T_width();x++)
                    fprintf( stderr, "%3ld ", delta_[x*H_+y] );
                fprintf( stderr, "\n" );
            }
        }
        if (verbose_)
            std::clog << "]\n";

        auto runs = compress( budget, target_data_, delta_ );

        for (auto &run:runs)
            if (run.offset>=get_T_size())
            {
                std::clog << "\n\n" << sizeof(T) << ": at " << run.offset << " " << run.data.size() << " data elements\n";
                assert( run.offset<get_T_size() );
            }

            //  Encode the runs
        std::vector<uint8_t> res;

        const int max_run_len = 127;
            //  Runs must not contain more than 256 bytes
        std::vector<run<T>> smaller;
        for (auto &run:runs)
        {
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

        if (sizeof(T)==4)
            closer = runs;

        if (verbose_)
        {
            std::sort( std::begin(closer), std::end(closer), [](auto &a, auto &b) {return a.offset < b.offset; } );

            std::clog << closer.size() << " runs of " << sizeof(T)*8 << "bytes = ";
            size_t item_count = 0;
            size_t bits_changed = 0;
            
            for (auto &run:closer)
            {
                std::clog << "@" << run.offset*sizeof(T) << ":[ ";
                for (int i=0;i!=run.data.size();i++)
                {

                    auto offset = vertical_from_horizontal( run.offset )+i;

                    if (sizeof(T)==2)
                        fprintf( stderr, "%04x ", run.data[i]^current_data_[offset] );
                    else
                    {
                        fprintf(
                            stderr,
                            "%08x ",
                            run.data[i]^current_data_[offset]
                            );
                    }

                    bits_changed += mypopcount( (T)(run.data[i]^current_data_[offset]) );
                }
                std::clog << "]  ";
                item_count += run.data.size();
            }
            std::clog << "=> " << item_count << " changed " << bits_changed << " bits\n";
        }

            //  Encode Z32
        if (sizeof(T)==4)
        {
            for (auto &run:closer)
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
        }

        current = framebuffer{current_data_, W_, H_};

        return res;
    }
};

#endif
