#include "streaming_session.hpp"

#include "../encode_frame.hpp"

#include <cassert>

namespace macflim
{

streaming_session::streaming_session(frame_source source, std::vector<codec_spec> codecs, size_t max_byterate,
                                     std::unique_ptr<transport> tp, size_t width, size_t height)
    : source_{std::move(source)}, codecs_{std::move(codecs)}, transport_{std::move(tp)},
      tracker_{[&]
               {
                   bitmap initial(width, height);
                   initial.fill(0xFF); // Encoder assumes all-black starting state
                   return initial;
               }()},
      rate_ctrl_{max_byterate}
{
    assert(!codecs_.empty());
}

step_result streaming_session::step()
{
    //  Drain any pending feedback first
    process_pending_feedback();

    //  Get next target bitmap from the source
    auto target = source_();
    if (!target)
        return step_result::finished;

    //  Encode against best-guess client screen
    bitmap client_fb = tracker_.current_client_screen();
    size_t budget = rate_ctrl_.budget_for_next_frame();
    encoding_result result = encode_frame(client_fb, *target, codecs_, budget);

    //  Build and send frame packet
    auto delta = result.get_video_encoded_data();
    frame_header hdr;
    hdr.seq = tracker_.next_seq();
    hdr.ticks = 1;
    transport_->send_frame(hdr, delta);

    //  Record delta in tracker for client state simulation
    tracker_.record_sent(std::move(delta));

    return step_result::ok;
}

void streaming_session::process_pending_feedback()
{
    while (auto fb = transport_->receive_feedback())
    {
        //  Extract history for the rate controller: each bit in the history
        //  represents a frame outcome (1=displayed, 0=missed)
        uint32_t last_seq = fb->last_displayed_seq;
        uint32_t base_seq = tracker_.simulated_seq();

        //  Feed outcomes to rate controller for frames we haven't confirmed yet
        if (last_seq > base_seq)
        {
            std::vector<uint8_t> history_vec(fb->history.begin(), fb->history.end());
            size_t num_new = last_seq - base_seq;
            for (size_t i = 0; i < num_new && i < HISTORY_BITS; ++i)
            {
                //  Bit i corresponds to seq (last_seq - i)
                //  Walk from oldest new to newest: seq (base_seq+1) to last_seq
                size_t bit_index = last_seq - base_seq - 1 - i;
                if (bit_index < HISTORY_BITS)
                {
                    bool displayed = (history_vec[bit_index / 8] >> (bit_index % 8)) & 1;
                    rate_ctrl_.record_outcome(displayed);
                }
            }
        }

        //  Update tracker with the feedback
        std::vector<uint8_t> history_vec(fb->history.begin(), fb->history.end());
        tracker_.process_feedback(last_seq, history_vec);
    }
}

session_stats streaming_session::stats() const
{
    return {.frames_sent = tracker_.next_seq() - 1,
            .feedbacks_processed = 0, // Could track this if needed
            .current_byterate = rate_ctrl_.current_byterate(),
            .in_flight = tracker_.in_flight_count()};
}

bitmap streaming_session::client_screen() const
{
    return tracker_.current_client_screen();
}

} // namespace macflim
