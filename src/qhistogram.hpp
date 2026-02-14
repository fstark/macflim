#pragma once

#include <format>
#include <vector>
#include <cstddef>
#include <cassert>
#include <iostream>

namespace macflim
{

    template <size_t N>
    class qhistogram
    {
        size_t total_ = 0;
        std::vector<size_t> samples_;
        bool verbose_ = true;

    public:
        qhistogram() : samples_(N + 1) {}

        ~qhistogram()
        {
            dump();
        }

        void add(double quality)
        {
            assert(quality >= 0 && quality <= 1);
            samples_[quality * N]++;
            total_++;
        }

        void dump() const
        {
            if (verbose_)
            {
                std::clog << std::format("+----------+--------+----------+----------+\n");
                std::clog << std::format("|     Q    | Frames |   Perc.  |  Cumul.  |\n");
                std::clog << std::format("|----------|--------|----------|----------|\n");
            }

            size_t cumulative = 0;
            double var99 = 0;
            double var98 = 0;
            double var95 = 0;
            for (size_t i = 0; i != N + 1; i++)
            {
                cumulative += samples_[i];
                auto percent = (cumulative * 1.0 / total_);
                if (percent > 0.01 && var99 == 0)
                    var99 = i * 1.0 / N;
                if (percent > 0.02 && var98 == 0)
                    var98 = i * 1.0 / N;
                if (percent > 0.05 && var95 == 0)
                    var95 = i * 1.0 / N;
                if (verbose_)
                    if (samples_[i])
                        std::cerr << std::format("| {:7.3f}% | {:6} | {:7.3f}% | {:7.3f}% |\n",
                                                 i * 1.0 / N * 100, samples_[i],
                                                 (samples_[i] * 1.0 / total_) * 100, percent * 100);
            }
            if (verbose_)
                std::clog << std::format("+----------+--------+----------+----------+\n");
            std::clog << std::format("99% of the frames are within {}% of the target pixels\n", var99 * 100);
            std::clog << std::format("98% of the frames are within {}% of the target pixels\n", var98 * 100);
            std::clog << std::format("95% of the frames are within {}% of the target pixels\n", var95 * 100);
        }
    };

} // namespace macflim
