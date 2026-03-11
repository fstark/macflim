#pragma once

#include <cstddef>
#include <deque>

namespace macflim
{

/// Adjusts per-frame byte budget using AIMD (additive increase, multiplicative decrease).
/// Fed frame outcomes (displayed/missed) from feedback processing, it reduces budget when
/// loss exceeds 10% over a 1-second window, and cautiously increases when the link is clean.
class adaptive_rate_controller
{
  public:
    explicit adaptive_rate_controller(size_t max_byterate);

    /// Byte budget to use for the next frame's encoding.
    [[nodiscard]] size_t budget_for_next_frame() const;

    /// Record that a frame was displayed (true) or missed (false).
    /// Must be called once per confirmed frame, in sequence order.
    void record_outcome(bool displayed);

    [[nodiscard]] size_t current_byterate() const;
    [[nodiscard]] size_t max_byterate() const;

  private:
    void try_decrease();
    void try_increase();

    static constexpr size_t WINDOW = 60;              // 1 second at 60 fps
    static constexpr size_t INCREASE_THRESHOLD = 120; // 2 full clean windows before increase
    static constexpr size_t MIN_BYTERATE = 8;         // Enough for a null codec header

    size_t max_byterate_;
    size_t current_byterate_;
    std::deque<bool> outcomes_;    // Sliding window of recent frame outcomes
    size_t consecutive_clean_ = 0; // Consecutive displayed frames (resets on any miss)
};

} // namespace macflim
