#include "client_state_tracker.hpp"

#include "../decoder.hpp"

namespace macflim
{

namespace
{

/// Check if a frame was displayed according to the history bitmap.
/// Bit 0 of byte 0 = last_displayed_seq, bit 1 = last_displayed_seq-1, etc.
bool was_displayed(uint32_t frame_seq, uint32_t last_displayed_seq, const std::vector<uint8_t> &history_bytes)
{
    uint32_t bit_index = last_displayed_seq - frame_seq;
    size_t byte_index = bit_index / 8;
    uint8_t bit_mask = 1u << (bit_index % 8);

    //  Frame is beyond the history window — assume missed
    if (byte_index >= history_bytes.size())
        return false;

    return (history_bytes[byte_index] & bit_mask) != 0;
}

} // namespace

client_state_tracker::client_state_tracker(bitmap initial_screen) : simulated_fb_{std::move(initial_screen)} {}

bitmap client_state_tracker::current_client_screen() const
{
    //  Start from confirmed state and optimistically apply all in-flight deltas
    bitmap screen = simulated_fb_;
    for (const auto &frame : in_flight_)
        apply_delta(screen, frame.delta);
    return screen;
}

void client_state_tracker::record_sent(uint32_t seq, std::vector<uint8_t> delta)
{
    in_flight_.push_back({seq, std::move(delta)});
}

void client_state_tracker::process_feedback(uint32_t last_displayed_seq, const std::vector<uint8_t> &history_bytes)
{
    //  Stale or duplicate feedback — nothing to do
    if (last_displayed_seq <= simulated_seq_)
        return;

    //  Replay deltas from simulated_fb_ forward, selectively applying based on history bitmap
    bitmap screen = simulated_fb_;
    while (!in_flight_.empty() && in_flight_.front().seq <= last_displayed_seq)
    {
        const auto &frame = in_flight_.front();

        if (was_displayed(frame.seq, last_displayed_seq, history_bytes))
            apply_delta(screen, frame.delta);
        //  Missed frames: skip — screen unchanged for that delta

        in_flight_.pop_front();
    }

    simulated_fb_ = std::move(screen);
    simulated_seq_ = last_displayed_seq;
}

uint32_t client_state_tracker::simulated_seq() const
{
    return simulated_seq_;
}

size_t client_state_tracker::in_flight_count() const
{
    return in_flight_.size();
}

} // namespace macflim
