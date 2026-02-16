#pragma once

#include <vector>
#include <cstddef>
#include "grayscale.hpp"
#include "subtitles.hpp"

namespace macflim
{

    class subtitle_burner
    {
        std::vector<subtitle> subtitles_; //  Copy of the subtitle list
        size_t current_index_ = 0;        //  Index of current subtitle

    public:
        subtitle_burner(const std::vector<subtitle> &subtitles) : subtitles_{subtitles}
        {
        }

        //  Burn the subtitle for time into the image
        void burn_into(grayscale &img, double time)
        {
            if (current_index_ < subtitles_.size())
            {
                const auto &current_subtitle = subtitles_[current_index_];

                if (time >= current_subtitle.start)
                {
                    if (time < current_subtitle.stop)
                    {
                        if (!current_subtitle.text.empty())
                        {
                            burn_subtitle(img, current_subtitle.text.front());
                        }
                    }
                    else
                    {
                        current_index_++; // Move to next subtitle
                    }
                }
            }
        }
    };

} // namespace macflim
