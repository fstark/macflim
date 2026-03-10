#pragma once

#include "../bitmap.hpp"

#include <cstdint>
#include <deque>
#include <vector>

namespace macflim
{

/// Maintains a pixel-perfect simulation of the client's screen state.
/// The server encodes deltas against current_client_screen(). When feedback arrives
/// reporting missed frames, the tracker replays deltas selectively — applying received
/// ones, skipping missed ones — to reconstruct exactly what the client has on screen.
class client_state_tracker
{
  public:
    explicit client_state_tracker(bitmap initial_screen);

    /// Best guess of the client's current screen: simulated_fb_ + all in-flight deltas applied.
    [[nodiscard]] bitmap current_client_screen() const;

    /// Record that we just sent a frame with this encoded delta.
    /// Frames are implicitly numbered: the first after construction is simulated_seq()+1, etc.
    void record_sent(std::vector<uint8_t> delta);

    /// Process client feedback: replay deltas selectively to reconstruct the client's true screen.
    /// history_bytes is the bitmap from the feedback packet — bit 0 of byte 0 corresponds to
    /// last_displayed_seq, bit 1 to last_displayed_seq-1, etc. 1=displayed, 0=missed.
    void process_feedback(uint32_t last_displayed_seq, const std::vector<uint8_t> &history_bytes);

    /// Sequence number that will be assigned to the next record_sent() call.
    [[nodiscard]] uint32_t next_seq() const;

    /// Last confirmed sequence number.
    [[nodiscard]] uint32_t simulated_seq() const;

    /// Number of frames currently in flight (sent but not yet confirmed).
    [[nodiscard]] size_t in_flight_count() const;

  private:
    bitmap simulated_fb_;
    uint32_t simulated_seq_ = 0;
    std::deque<std::vector<uint8_t>> in_flight_; // consecutive deltas, starting at simulated_seq_+1
};

} // namespace macflim
