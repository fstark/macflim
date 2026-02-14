#pragma once

#include <vector>
#include "grayscale.hpp"
#include "subtitles.hpp"

namespace macflim
{

    class SubtitleBurner
    {
        std::vector<subtitle> subtitles_; //  The subtitles to burn
                                          // #### Should be a pair of const_iterators

    public:
        SubtitleBurner(const std::vector<subtitle> &subtitles) : subtitles_{subtitles}
        {
        }

        //  Burn the subtitle for time into the image;
        void burn_into(grayscale &img, double time)
        {
            if (subtitles_.size() > 0)
            {
                //  Can do that way better with an iterator!
                if (time >= subtitles_.front().start)
                {
                    if (time < subtitles_.front().stop)
                    {
                        ::burn_subtitle(img, subtitles_.front().text.front()); //  #### zero line subtitles will crash
                    }
                    else
                    {
                        subtitles_.erase(subtitles_.begin()); //  We should flip the subtitles order in constructor!
                    }
                }
            }
        }
    };

} // namespace macflim
