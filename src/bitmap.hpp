#pragma once

#include <bit>
#include <cstdint>
#include <cstring>
#include <random>
#include <vector>

#include "grayscale.hpp"

namespace macflim
{

template <typename T> std::array<uint8_t, sizeof(T)> bytes_from_value_be(T v)
{
    std::array<uint8_t, sizeof(T)> res;
    for (int i = 0; i != sizeof(T); i++)
    {
        res[sizeof(T) - i - 1] = v & 0xff;
        v >>= 8;
    }
    return res;
}

template <typename T> std::vector<uint8_t> bytes_from_values_be(const std::vector<T> &values)
{
    std::vector<uint8_t> res;
    for (auto &v : values)
    {
        auto a = bytes_from_value_be(v);
        res.insert(std::end(res), std::begin(a), std::end(a));
    }
    return res;
}

//  unpack a single value
template <typename T> void copy_bytes_from_value_be(std::vector<uint8_t>::iterator p, T v)
{
    for (int i = 0; i != sizeof(T); i++)
    {
        p[sizeof(T) - i - 1] = v & 0xff;
        v >>= 8;
    }
}

//  Specialization for uint8_t - no conversion needed
template <> inline void copy_bytes_from_value_be<uint8_t>(std::vector<uint8_t>::iterator p, uint8_t v)
{
    *p = v;
}

//  Unpack into q, with arbitrary stride, increments p
template <typename IT>
void copy_bytes_from_values_be(std::vector<uint8_t>::iterator destination, IT source, size_t count, size_t stride)
{
    while (count--)
    {
        copy_bytes_from_value_be(destination, *source++);
        destination += stride;
    }
}

///  A bitmap is a packed black and white screen
class bitmap
{
  private:
    std::vector<uint8_t> data_; //  Framebuffer content
    size_t W_;                  //  Width in pixels
    size_t H_;                  //  Height in pixels

    size_t get_rowbytes() const
    {
        return W_ / 8;
    }

    template <typename T> size_t get_width() const
    {
        return get_rowbytes() / sizeof(T);
    }

    template <typename T> size_t get_size() const
    {
        return get_width<T>() * H_;
    }

    void assert_size(const bitmap &other) const
    {
        assert(W_ == other.W_ && H_ == other.H_);
    }

    //  packs means extracting a value from the bitmap for calculations
    //  this value can generally a uint8_t, a uint16_t or a uint32_t

    //  the values are packed/unpacked in big endian, as it corresponds to the behavior on the Mac

    //  horizontal means consecutive values corresponds to horizontally adjacent pixels (logical layout of the hardware)
    //  vertical means consecutive values corresponds to verticall adjacent pixels (better for compression)

    //  Everything about writing values into the bitmap

    //  Unpack the whole buffer horizontally
    template <typename IT> void unpack_horizontal_be(std::vector<uint8_t>::iterator destination, IT source) const
    {
        copy_bytes_from_values_be(destination, source, get_size<typename IT::value_type>(),
                                  sizeof(typename IT::value_type));
    }

    //  Unpack the whole buffer vertically
    template <typename IT> void unpack_vertical_be(std::vector<uint8_t>::iterator destination, IT source) const
    {
        for (size_t i = 0; i != get_width<typename IT::value_type>(); i++)
        {
            copy_bytes_from_values_be(destination, source, H_, get_rowbytes());
            source += H_;
            destination += sizeof(typename IT::value_type);
        }
    }

    //  Everything about reading values from the bitmap

    //  Gets a value from n consecutive bytes in big-endian
    template <typename T> T value_from_bytes_be(std::vector<uint8_t>::const_iterator source) const
    {
        T v = 0;

        for (int i = 0; i != sizeof(T); i++)
            v = (v << 8) + (*source++);

        return v;
    }

    //  Extract count items, separated by stride bytes, into destination
    //  performing the right endianness type conversion
    template <typename T, typename IT>
    void copy_from_bytes_be(IT destination, std::vector<uint8_t>::const_iterator source, size_t count,
                            size_t stride) const
    {
        while (count--)
        {
            *destination++ = value_from_bytes_be<T>(source);
            source += stride;
        }
    }

    //  Packs the whole buffer horizontally
    template <typename T, typename IT> void pack_horizontal_be(IT out) const
    {
        copy_from_bytes_be<T, IT>(out, std::begin(data_), get_size<T>(), 1);
    }

    //  Packs the whole buffer vertically
    template <typename T, typename IT> void pack_vertical_be(IT out) const
    {
        for (size_t x = 0; x != get_rowbytes(); x += sizeof(T))
            copy_from_bytes_be<T, IT>(out, std::begin(data_) + x, H_, get_rowbytes());
    }

  public:
    bitmap(size_t W, size_t H) : data_(W * H / 8), W_{W}, H_{H}
    {
        std::fill(std::begin(data_), std::end(data_), 0xf0);
    }

    bitmap(const grayscale &img) : data_(img.W() * img.H() / 8), W_{img.W()}, H_{img.H()}
    {
        assert(img.W() == W_ && img.H() == H_);
        auto p = std::begin(data_);
        for (size_t y = 0; y != H_; y++)
            for (size_t x = 0; x != W_; x += 8)
                *p++ =
                    (int)(img.at(x, y) * 128 + img.at(x + 1, y) * 64 + img.at(x + 2, y) * 32 + img.at(x + 3, y) * 16 +
                          img.at(x + 4, y) * 8 + img.at(x + 5, y) * 4 + img.at(x + 6, y) * 2 + img.at(x + 7, y)) ^
                    0xff;
    }

    template <typename T>
    bitmap(const std::vector<T> &data, size_t W, size_t H, bool vertical = true) : data_(W * H / 8), W_{W}, H_{H}
    {
        if (vertical)
        {
            unpack_vertical_be(std::begin(data_), std::begin(data));
        }
        else
        {
            unpack_horizontal_be(std::begin(data_), std::begin(data));
        }
    }

    ~bitmap() {}

    void fill(uint8_t value)
    {
        std::fill(std::begin(data_), std::end(data_), value);
    }

    void randomize(int seed)
    {
        std::default_random_engine generator(seed);
        std::uniform_int_distribution<int> distribution(0, 255);
        for (auto &v : data_)
            v = distribution(generator);
    }

    size_t W() const
    {
        return W_;
    }
    size_t H() const
    {
        return H_;
    }

    template <typename T> std::vector<T> raw_vertical() const
    {
        std::vector<T> res;
        pack_vertical_be<T, decltype(std::back_inserter(res))>(std::back_inserter(res));

        return res;
    }

    template <typename T> std::vector<T> raw_values() const
    {
        return raw_vertical<T>();
    }

    template <typename T> std::vector<T> raw_values_natural() const
    {
        std::vector<T> res;
        pack_horizontal_be<T, decltype(std::back_inserter(res))>(std::back_inserter(res));

        return res;
    }

    grayscale as_image() const
    {
        grayscale res(W_, H_);
        for (size_t y = 0; y != H_; y++)
            for (size_t x = 0; x != W_; x++)
                res.at(x, y) = !(data_[y * get_rowbytes() + x / 8] & (1 << (7 - (x % 8))));
        return res;
    }

    bool operator==(const bitmap &o) const
    {
        return H_ == o.H_ && W_ == o.W_ && data_ == o.data_;
    }

    bitmap operator^(const bitmap &o) const
    {
        assert(W_ == o.W_ && H_ == o.H_);
        bitmap result(W_, H_);
        for (size_t i = 0; i != data_.size(); i++)
            result.data_[i] = data_[i] ^ o.data_[i];
        return result;
    }

    std::vector<uint8_t> raw_data() const
    {
        std::vector<uint8_t> result;
        for (size_t i = 0; i != data_.size(); i++)
            result.push_back(data_[i]);
        return result;
    }

    size_t pixel_count() const
    {
        size_t count = 0;
        for (auto &v : data_)
            count += mypopcount(v);
        return count;
    }

    size_t count_differences(const bitmap &other) const
    {
        return (*this ^ other).pixel_count();
    }

    double proximity(const bitmap &other) const
    {
        return 1 - (count_differences(other) / (double)(W_ * H_));
    }

    void invert()
    {
        for (auto &v : data_)
            v ^= 0xff;
    }

    bitmap inverted() const
    {
        bitmap res = *this;
        res.invert();
        return res;
    }

    void copy_lines_from(const bitmap &other, size_t from, size_t count)
    {
        assert_size(other);
        assert(from < H_);
        assert(from + count <= H_);
        memcpy(data_.data() + from * get_rowbytes(), other.data_.data() + from * get_rowbytes(),
               count * get_rowbytes());
    }

    template <typename T> void extract(T out, size_t x, size_t y, size_t bytelen) const
    {
        auto p = std::begin(data_) + x + y * get_rowbytes();
        std::copy(p, p + bytelen, out);
    }
};

} // namespace macflim
