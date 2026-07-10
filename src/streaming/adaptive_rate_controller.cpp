#include "adaptive_rate_controller.hpp"

#include <algorithm>

namespace macflim
{

adaptive_rate_controller::adaptive_rate_controller(size_t max_byterate)
    : max_byterate_{max_byterate}, current_byterate_{max_byterate * 3 / 4}
{
    //  Start at 75% of max — aggressive enough to be useful quickly,
    //  conservative enough to absorb initial jitter
    current_byterate_ = std::max(current_byterate_, MIN_BYTERATE);
}

size_t adaptive_rate_controller::budget_for_next_frame() const
{
    return current_byterate_;
}

void adaptive_rate_controller::record_outcome(bool displayed)
{
    outcomes_.push_back(displayed);
    if (outcomes_.size() > WINDOW)
        outcomes_.pop_front();

    if (displayed)
    {
        ++consecutive_clean_;
        try_increase();
    }
    else
    {
        ++frames_dropped_;
        consecutive_clean_ = 0;
        try_decrease();
    }
}

void adaptive_rate_controller::try_decrease()
{
    //  Only check when we have a full window
    if (outcomes_.size() < WINDOW)
        return;

    auto misses = std::ranges::count(outcomes_, false);

    //  Loss ratio > 10% → multiplicative decrease
    if (misses * 10 > static_cast<long>(WINDOW))
    {
        current_byterate_ = std::max(MIN_BYTERATE, current_byterate_ * 80 / 100);
        //  Clear window to prevent repeated decreases from the same loss event
        outcomes_.clear();
        consecutive_clean_ = 0;
    }
}

void adaptive_rate_controller::try_increase()
{
    if (consecutive_clean_ < INCREASE_THRESHOLD)
        return;

    //  120 consecutive clean frames → cautious 5% increase
    size_t increased = current_byterate_ * 105 / 100;
    //  Ensure at least +1 byte of progress when current_byterate_ is small
    if (increased == current_byterate_)
        ++increased;
    current_byterate_ = std::min(max_byterate_, increased);
    consecutive_clean_ = 0;
}

size_t adaptive_rate_controller::current_byterate() const
{
    return current_byterate_;
}

size_t adaptive_rate_controller::max_byterate() const
{
    return max_byterate_;
}

size_t adaptive_rate_controller::frames_dropped() const
{
    return frames_dropped_;
}

} // namespace macflim
