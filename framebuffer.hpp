#ifndef FRAMEBUFFER_INCLUDED__
#define FRAMEBUFFER_INCLUDED__

#include <vector>
#include "image.hpp"
#include <cstdint>


//  ------------------------------------------------------------------
//  A framebuffer is a packed black and white screen
//  One can xor framebuffers together
//  and get their encoded version in a row-major or column-major way
//  as 8, 16 or 32 bits buffers in big-endian
//  ------------------------------------------------------------------

class framebuffer
{
private:
    std::vector<uint8_t> data_;     //  Framebuffer content
    size_t W_;                      //  Width in pixels
    size_t H_;                      //  Height in pixels

    size_t get_rowbytes() const { return W_/8; }
 
    template <typename T>
    size_t get_width() const { return get_rowbytes()/sizeof(T); }

    template <typename T>
    size_t get_size() const { return get_width<T>()*H_; }

        //  packs means extracting a value from the framebuffer for calculations
        //  this value can generally a uint8_t, a uint16_t or a uint32_t

        //  the values are packed/unpacked in big endian, as it corresponds to the behavior on the Mac

        //  horizontal means consecutive values corresponds to horizontally adjacent pixels (logical layout of the hardware)
        //  vertical means consecutive values corresponds to verticall adjacent pixels (better for compression)

        //  Everything about writing values into the framebuffer

        //  unpack a single value
    template <typename T>
    void copy_from_value_be( std::vector<uint8_t>::iterator p, T v ) const
    {
        for (int i=0;i!=sizeof(T);i++)
        {
            p[sizeof(T)-i-1] = v & 0xff;
            v >>= 8;
        }
    }

        //  Unpack into q, with arbitrary stride, increments p
    template <typename IT>
    void copy_from_values_be( std::vector<uint8_t>::iterator destination, IT source, size_t count, size_t stride ) const
    {
        while (count--)
        {
            copy_from_value_be( destination, *source++ );
            destination += stride;
        }
    }

        //  Unpack the whole buffer horizontally
    template <typename IT>
    void unpack_horizontal_be( std::vector<uint8_t>::iterator destination,  IT source ) const
    {
        copy_from_values_be( destination, source, get_size<typename IT::value_type>(), sizeof(typename IT::value_type) );
    }

        //  Unpack the whole buffer vertically
    template <typename IT>
    void unpack_vertical_be( std::vector<uint8_t>::iterator destination,  IT source ) const
    {
        for (int i=0;i!=get_width<typename IT::value_type>();i++)
        {
            copy_from_values_be( destination, source, H_, get_rowbytes() );
            source += H_;
            destination += sizeof(typename IT::value_type);
        }
    }

        //  Everything about reading values from the framebuffer

        //  Gets a value from n consecutive bytes in big-endian
    template <typename T>
    T value_from_bytes_be( std::vector<uint8_t>::const_iterator source ) const
    {
        T v = 0;
        
        for (int i=0;i!=sizeof(T);i++)
            v  = (v<<8) + (*source++);

        return v;
    }

        //  Extract count items, separated by stride bytes, into destination
        //  performing the right endianness type conversion
    template <typename T, typename IT>
    void copy_from_bytes_be( IT destination, std::vector<uint8_t>::const_iterator source, size_t count, size_t stride ) const
    {
        while (count--)
        {
            *destination++ = value_from_bytes_be<T>( source );
            source += stride;
        }
    }

        //  Packs the whole buffer horizontally
    template <typename T, typename IT>
    void pack_horizontal_be( IT out ) const
    {
        copy_from_bytes_be<T,IT>( out, std::begin(data_), get_size<T>(), 1 );
    }

        //  Packs the whole buffer vertically
    template <typename T, typename IT>
    void pack_vertical_be( IT out ) const
    {
        for (int x=0;x!=get_rowbytes();x+=sizeof(T))
            copy_from_bytes_be<T,IT>(
                out,
                std::begin(data_)+x,
                H_,
                get_rowbytes()
                );
    }

public:
    framebuffer( size_t W, size_t H ) : data_( W*H/8 ), W_{W}, H_{H}
    {
        std::fill( std::begin(data_), std::end(data_), 0 );
    }

    framebuffer( const image &img ) : data_( img.W()*img.H()/8 ), W_{ img.W() }, H_{ img.H() }
    {
        assert( img.W()==W_ && img.H()==H_ );
        auto p = std::begin(data_);
        for (int y=0;y!=H_;y++)
            for (int x=0;x!=W_;x+=8)
                *p++ = (int)(img.at(x  ,y)*128+img.at(x+1,y)*64+img.at(x+2,y)*32+img.at(x+3,y)*16+
                             img.at(x+4,y)*  8+img.at(x+5,y)* 4+img.at(x+6,y)* 2+img.at(x+7,y)     ) ^ 0xff;
    }

    template <typename T>
    framebuffer( const std::vector<T> &data, size_t W, size_t H, bool vertical=true ) : data_( W*H/8 ), W_{W}, H_{H}
    {
        if (vertical)
        {
            unpack_vertical_be( std::begin(data_), std::begin(data) );
        }
        else
        {
            unpack_horizontal_be( std::begin(data_), std::begin(data) );
        }
    }

    ~framebuffer()
    {
    }

    size_t W() const { return W_; }
    size_t H() const { return H_; }

    template <typename T>
    std::vector<T> raw_vertical() const
    {
        std::vector<T> res;
        pack_vertical_be<T,decltype(std::back_inserter(res))>( std::back_inserter(res) );

        return res;
    }

    template <typename T>
    std::vector<T> raw_values() const
    {
        return raw_vertical<T>();
    }

    image as_image() const
    {
        image res( W_, H_ );
        for (int y=0;y!=H_;y++)
            for (int x=0;x!=W_;x++)
                res.at(x,y) = !(data_[y*get_rowbytes()+x/8] & (1<<(7-(x%8))));
        return res;
    }

    framebuffer operator^(const framebuffer &o)
    {
        assert( W_==o.W_ && H_==o.H_ );
        framebuffer result( W_, H_ );
        for (size_t i=0;i!=data_.size();i++)
            result.data_[i] = data_[i] ^ o.data_[i];
        return result;
    }

    std::vector<uint8_t> raw_data() const
    {
        std::vector<uint8_t> result;
        for (size_t i=0;i!=data_.size();i++)
            result.push_back( data_[i] );
        return result;
    }
};

#endif
