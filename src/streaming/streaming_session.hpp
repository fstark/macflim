#pragma once

/// Server-side streaming session: reads target bitmaps from a frame_source,
/// encodes deltas against the tracked client screen, sends them via transport,
/// and processes feedback to maintain pixel-perfect client state simulation.
///
/// The session is driven by step() calls — each call processes one frame tick.
/// This avoids owning a thread or timer, letting the caller control pacing.

#include "adaptive_rate_controller.hpp"
#include "client_state_tracker.hpp"
#include "frame_source.hpp"
#include "protocol.hpp"
#include "transport.hpp"

#include "../codec_spec.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace macflim
{

/// Result of a single step() call.
enum class step_result
{
    ok,       // Frame encoded and sent
    finished, // Frame source exhausted
};

/// Statistics exposed for monitoring and testing.
struct session_stats
{
    uint32_t frames_sent = 0;
    uint32_t feedbacks_processed = 0;
    size_t current_byterate = 0;
    size_t in_flight = 0;
};

/// Server-side streaming session orchestrator.
class streaming_session
{
  public:
    streaming_session(frame_source source, std::vector<codec_spec> codecs, size_t max_byterate,
                      std::unique_ptr<transport> transport, size_t width, size_t height);

    /// Process one frame tick: encode next target, send it, drain feedback.
    /// Returns step_result::finished when the frame source is exhausted.
    [[nodiscard]] step_result step();

    /// Drain all pending feedback from the transport and update internal state.
    void process_pending_feedback();

    /// Current session statistics.
    [[nodiscard]] session_stats stats() const;

    /// The tracker's current best guess of the client screen (for testing/monitoring).
    [[nodiscard]] bitmap client_screen() const;

  private:
    frame_source source_;
    std::vector<codec_spec> codecs_;
    std::unique_ptr<transport> transport_;

    client_state_tracker tracker_;
    adaptive_rate_controller rate_ctrl_;
};

} // namespace macflim
