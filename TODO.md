Goal: have a way to encode a flim without storing all of it in memory.

Idea:

use coroutines to read the stream and yield data packets as they are decoded. Each data packet contains a buffer of video frames and a single audio frame. Here is a sample that works on both gcc and clang (std==c++23).
I don't think it is correct, our generation doesn't always have the same number of video frames between audio frames, but it is an idea.

the coroutine would be consumed by the encoder.

#include <cppcoro/generator.hpp>
#include <iostream>
#include <random>
#include <tuple>
#include <vector>

using video_frame = int;
using audio_frame = double;
using data_packet = std::tuple<std::vector<video_frame>, audio_frame>;

class stream_reader {
   public:
    stream_reader(std::size_t total_length) : stream_length(total_length){};

    video_frame next_video_frame();
    audio_frame next_audio_frame();

    bool finished() const;
    bool next_frame_is_audio() const;

   private:
    std::size_t const stream_length;  // Total stream length
    std::size_t current_position{0};  // Current position in the total stream
    std::size_t video_frame_count{0}; // Num video frames since last audio frame
    std::size_t audio_delay{0};       // Num video frames until next audio frame
    
    // Random number generators for audio samples, video samples, and variable frame rate
    std::random_device r;
    std::default_random_engine rng_engine{r()};
    std::uniform_int_distribution<video_frame> video_rng{-100, 100};
    std::uniform_real_distribution<audio_frame> audio_rng{-1.0, 1.0};
    std::uniform_int_distribution<std::size_t> frame_delay_rng{1, 10};
};

cppcoro::generator<data_packet> decoder(stream_reader& reader) {

    std::vector<video_frame> buffer{};

    for (; !reader.finished(); ) {
        if (reader.next_frame_is_audio()) {
            co_yield data_packet{buffer, reader.next_audio_frame()};
            buffer.clear();
        }
        else
        {
            buffer.push_back(reader.next_video_frame());
        }
    }
}

int main() {
    stream_reader reader{100};

    for (auto const& [video_buffer, audio_buffer] : decoder(reader)) {
        std::cout << "Audio " << audio_buffer << "\nVideo ";

        for (auto frame : video_buffer) {
            std::cout << frame << ",";
        }

        std::cout << "\n";
    }
    return 0;
}

video_frame stream_reader::next_video_frame() {
    ++video_frame_count;
    ++current_position;
    return video_rng(rng_engine);
}

audio_frame stream_reader::next_audio_frame() {
    audio_delay = frame_delay_rng(rng_engine);
    video_frame_count = 0;
    ++current_position;
    return audio_rng(rng_engine);
}

bool stream_reader::finished() const {
    return current_position >= stream_length;
}

bool stream_reader::next_frame_is_audio() const {
    return audio_delay == video_frame_count;
}