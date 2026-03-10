#include "encode_frame.hpp"

#include <algorithm>
#include <cassert>

namespace macflim
{

encoding_result encode_frame(bitmap &current_fb, const bitmap &target, const std::vector<codec_spec> &codecs,
                             size_t budget)
{
    assert(!codecs.empty());

    std::vector<encoding_result> results;
    results.reserve(codecs.size());
    std::transform(codecs.begin(), codecs.end(), std::back_inserter(results), [&](const auto &codec) -> encoding_result
                   { return encoding_result(codec, current_fb, target, budget); });

    auto best =
        std::max_element(results.begin(), results.end(), [](const encoding_result &r1, const encoding_result &r2)
                         { return r1.quality() < r2.quality(); });

    current_fb = best->image();
    return *best;
}

} // namespace macflim
