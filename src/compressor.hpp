#pragma once

#include "bitmap.hpp"
#include "common.hpp"
#include "decoder.hpp"
#include "imgcompress.hpp"
#include "ruler.hpp"

#include <algorithm>
#include <bit>
#include <bitset>
#include <cstdint>
#include <format>
#include <iostream>
#include <limits>
#include <numeric>
#include <vector>

namespace macflim
{

/**
 * Encapsulate a way to compress a single frame transition
 */
class compressor
{
  protected:
    size_t W_;
    size_t H_;

    bool verbose_ = false;

    /// Width in bytes
    size_t get_bytes_width() const
    {
        return W_ / 8;
    }

    static size_t size_t_from(const std::string &v)
    {
        return std::stoull(v);
    }

  public:
    compressor(size_t width, size_t height) : W_{width}, H_{height} {}

    virtual ~compressor() {}
    [[nodiscard]] virtual std::vector<uint8_t> compress(bitmap &current, const bitmap &target,
                                                        /* weigths, */ size_t budget) const = 0;

    virtual bool set_parameter(const std::string parameter, const std::string value)
    {
        if (parameter == "verbose")
            verbose_ = bool_from(value);
        return false;
    }

    [[nodiscard]] virtual std::string name() const = 0;

    [[nodiscard]] virtual std::string description() const
    {
        return name();
    }
};

/// Null compressor that makes no changes to the current bitmap.
class null_compressor final : public compressor
{
    std::string name() const override
    {
        return "null";
    };

  public:
    null_compressor(size_t width, size_t height) : compressor{width, height} {}

    std::vector<uint8_t> compress([[maybe_unused]] bitmap &current, [[maybe_unused]] const bitmap &target,
                                  /* weigths, */ [[maybe_unused]] size_t budget) const override
    {
        return {};
    }
};

/// Invert compressor that flips all pixels in the current bitmap.
class invert_compressor final : public compressor
{
    std::string name() const override
    {
        return "invert";
    };

  public:
    invert_compressor(size_t width, size_t height) : compressor{width, height} {}

    std::vector<uint8_t> compress(bitmap &current, [[maybe_unused]] const bitmap &target,
                                  /* weigths, */ [[maybe_unused]] size_t budget) const override
    {
        current = current.inverted();
        return {};
    }
};

/// Copy line compressor that copies full scanlines from target to current bitmap.
class copy_line_compressor final : public compressor
{
    std::string name() const override
    {
        return "lines";
    };

    bool set_parameter(const std::string parameter, const std::string value) override
    {
        return compressor::set_parameter(parameter, value);
    }

    std::vector<uint8_t> compress(bitmap &current, const bitmap &target, /* weigths, */ size_t budget) const override
    {
        bitmap result{current};

        size_t q = 0;

        size_t line_start = 0;
        size_t line_count = 0;

        //  How many lines can we afford within the budget?
        size_t target_count = budget / get_bytes_width();

        //  Budget too small for even one line — nothing to do
        if (target_count == 0)
            return {};

        // std::clog << "Lines: " << budget << " bytes " << target_count << " lines \n";

        for (size_t i = 0; i < current.H(); i += target_count)
        {
            bitmap fb = current;
            size_t lc = std::min(target_count, current.H() - i);
            fb.copy_lines_from(target, i, lc);
            auto res = fb.count_differences(current);
            if (res > q)
            {
                q = res;
                result = fb;
                // std::clog << "[" << q << "]";
                line_start = i;
                line_count = lc;
            }
        }

        // std::clog << "COPY LINES : line_count == " << line_count << "  line_count " << line_count << "\n";
        // std::clog << "COPY LINES : bytes_count == " << line_count*get_bytes_width() << "  offset " <<
        // line_start*get_bytes_width() << "\n";

        current = result; //  #### THIS SEEMS VERY WRONG! WE SHOULD APPLY THE LINES TO CURRENT OR RETURN THE BEST FB
                          //  FROM ABOVE LOOP

        std::vector<uint8_t> data;
        auto out = std::back_inserter(data);

        write2(out, line_count * get_bytes_width());
        write2(out, line_start * get_bytes_width());

        target.extract(out, 0, line_start, line_count * get_bytes_width());

        return data;
    }

  public:
    copy_line_compressor(size_t width, size_t height) : compressor{width, height} {}
};

/**
 * Compresses an grayscale using vertical strips of various width
 */
template <typename T> class vertical_compressor : public compressor
{
    std::string name() const override
    {
        return std::format("z{}", sizeof(T) * 8);
    }

    const ruler<T> &ruler_;

    /// Width in underlying type
    [[nodiscard]] size_t get_T_width() const
    {
        return get_bytes_width() / sizeof(T);
    }

    /// Number of element of type for the whole screen
    [[nodiscard]] size_t get_T_size() const
    {
        return get_T_width() * H_;
    }

    std::vector<run<T>> compress(size_t max_size, const std::vector<T> &target_data_,
                                 const std::vector<size_t> &delta_) const
    {
        size_t header_size = sizeof(T) == 4 ? 4 : 2;

        packzmap packmap{get_T_size(), header_size, sizeof(T)};

        auto mx = *std::max_element(std::begin(delta_), std::end(delta_));

        std::vector<std::vector<size_t>> deltas;
        deltas.resize(mx + 1);

        for (size_t i = 0; i != get_T_size(); i++)
            if (delta_[i])
                deltas[delta_[i]].push_back(i);

        bool done = false;

        for (size_t i = deltas.size() - 1; i != 0; i--)
        {
            for (auto ix : deltas[i])
                if (packmap.set(ix) >= max_size)
                {
                    // std::clog << "Clearing at delta " << i << " size " << packmap.size() << " (index was:" << ix <<
                    // ")\n";
                    packmap.clear(ix);
                    done = true;
                    break;
                }

            if (done)
                break;

            //  We add the borders of the packmap if not "expensive"
            for (size_t ix = 0; ix != get_T_size(); ix++)
                if (((ix % H_) != 0) && ((ix % H_) != H_ - 1) && packmap.empty_border(ix))
                    if (delta_[ix] * 2 >= i)
                        if (packmap.set(ix) >= max_size)
                        {
                            packmap.clear(ix);
                            done = true;
                            break;
                        }

            if (done)
                break;
        }

        auto res = pack<T>(std::begin(target_data_), std::begin(packmap.mask()), std::end(packmap.mask()), max_size,
                           W_ / 8 / sizeof(T), H_);

        if (verbose_)
            std::clog << std::format("=> z{} Generated {} runs\n", sizeof(T) * 8, res.size());

        return res;
    }

  public:
    vertical_compressor(size_t W, size_t H, const ruler<T> &ruler) : compressor{W, H}, ruler_{ruler} {}

    size_t vertical_from_horizontal(size_t h) const
    {
        size_t offset = h * sizeof(T);

        assert(h < get_T_size());

        size_t scr_x = offset % get_bytes_width();
        size_t scr_y = offset / get_bytes_width();

        scr_x /= sizeof(T);

        offset = scr_x * H_ + scr_y;

        return offset;
    }

    std::vector<uint8_t> compress(bitmap &current, const bitmap &target, /* weigths, */ size_t budget) const override
    {
        // std::cerr << "BUDGET:" << budget << "\n";

        //  transient
        auto current_data_ =
            current.raw_values<T>(); //  The data present on screen (for optimisation purposes) (vertical)
        auto target_data_ = target.raw_values<T>(); //  The data we are trying to converge to
        std::vector<size_t> delta_(get_T_size());   //  0: it is sync'ed

        for (size_t i = 0; i != get_T_size(); i++)
        {
            if (current_data_[i] == target_data_[i])
                //  Data is identical, we don't care about updating this
                delta_[i] = 0;
            else
            {
                //  Let's increase the importance of updating this
                // delta_[i] += countbits( target_data_[i] ^ current_data_[i] );
                delta_[i] = ruler_.distance(target_data_[i], current_data_[i]);
            }
        }
        //  Display delta map in correct order
        if (verbose_)
        {
            std::clog << std::format("DELTA LIST OF {} elements: [\n", get_T_size());
            for (size_t y = 0; y != H_; y++)
            {
                std::cerr << std::format("    ");
                for (size_t x = 0; x != get_T_width(); x++)
                    std::cerr << std::format("{:3d} ", (long long)delta_[x * H_ + y]);
                std::cerr << std::format("\n");
            }
        }
        if (verbose_)
            std::clog << std::format("]\n");

        auto runs = compress(budget, target_data_, delta_);

        for (auto &run : runs)
            if (run.offset >= get_T_size())
            {
                std::clog << std::format("\n\n{}: at {} {} data elements\n", sizeof(T), run.offset, run.data.size());
                assert(run.offset < get_T_size());
            }

        //  Encode the runs
        std::vector<uint8_t> res;

        const int max_run_len = 127;
        //  Runs must not contain more than 256 bytes
        std::vector<run<T>> smaller;
        for (auto &run : runs)
        {
            //  #### : FIXME 32 words on screen
            auto rs = run.split(max_run_len, get_T_width());

            for (auto &run2 : rs)
                if (run2.offset >= get_T_size())
                {
                    std::clog << std::format("\n{}: at {} {} data elements split:\n", sizeof(T), run.offset,
                                             run.data.size());
                    std::clog << std::format("{}: at {} {} data elements\n", sizeof(T), run2.offset, run2.data.size());
                }

            smaller.insert(std::end(smaller), std::begin(rs), std::end(rs));
        }

        for (auto &run : smaller)
        {
            if (run.offset >= get_T_size())
                std::clog << "\n\n"
                          << sizeof(T) << ": at " << run.offset << " " << run.data.size() << " data elements\n";
            assert(run.offset < get_T_size());
        }

        //  Sorts the runs
        std::sort(std::begin(smaller), std::end(smaller), [](auto &a, auto &b) { return a.offset < b.offset; });

        //  Runs must be separated by at most 255 items
        std::vector<run<T>> closer;
        size_t offset = 0;
        for (auto &run : smaller)
        {
            while (run.offset - offset > 255)
            {
                offset += 255;
                closer.push_back({offset, {}});
            }
            closer.push_back(run);
            offset = run.offset;
        }

        for (auto &run : closer)
            assert(run.offset < get_T_size());

        if (sizeof(T) == 4)
            closer = runs;

        if (verbose_)
        {
            std::sort(std::begin(closer), std::end(closer), [](auto &a, auto &b) { return a.offset < b.offset; });

            std::clog << std::format("{} runs of {} bytes = ", closer.size(), sizeof(T) * 8);
            size_t item_count = 0;
            size_t bits_changed = 0;

            for (auto &run : closer)
            {
                std::clog << std::format("@{}: [ ", run.offset * sizeof(T));
                for (size_t i = 0; i != run.data.size(); i++)
                {

                    auto offset = vertical_from_horizontal(run.offset) + i;

                    if (sizeof(T) == 2)
                        std::cerr << std::format("{:04x} ", run.data[i] ^ current_data_[offset]);
                    else
                    {
                        std::cerr << std::format("{:08x} ", run.data[i] ^ current_data_[offset]);
                    }

                    bits_changed += mypopcount((T)(run.data[i] ^ current_data_[offset]));
                }
                std::clog << std::format("]  ");
                item_count += run.data.size();
            }
            std::clog << std::format("=> {} changed {} bits\n", item_count, bits_changed);
        }

        //  Encode Z32
        if (sizeof(T) == 4)
        {
            for (auto &run : closer)
            {
                uint32_t header = ((run.data.size() - 1) << 16) + ((run.offset + 1) * sizeof(T));

                auto v = bytes_from_value_be(header);
                res.insert(std::end(res), std::begin(v), std::end(v));
                auto vs = bytes_from_values_be(run.data);
                res.insert(std::end(res), std::begin(vs), std::end(vs));
            }
            res.push_back(0x00);
            res.push_back(0x00);
            res.push_back(0x00);
            res.push_back(0x00);

            apply_delta(current, 0x02, res.data(), res.size());
        }

        //  Encode Z16
        if (sizeof(T) == 2)
        {
            size_t z16_offset = 0;
            for (auto &run : closer)
            {
                uint16_t header = ((run.offset - z16_offset) << 8) + run.data.size(); //  oooooooo 0 sssssss
                z16_offset = run.offset;

                auto v = bytes_from_value_be(header);
                res.insert(std::end(res), std::begin(v), std::end(v));
                auto vs = bytes_from_values_be(run.data);
                res.insert(std::end(res), std::begin(vs), std::end(vs));
            }
            res.push_back(0x00);
            res.push_back(0x00);

            apply_delta(current, 0x01, res.data(), res.size());
        }

        return res;
    }
};

} // namespace macflim
