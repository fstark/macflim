#pragma once

//  A frame is the unit of data passed between the compressor and the flim file.
//  It contains encoded video delta and audio data for a single screen update.

#include "bitmap.hpp"
#include "imgcompress.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace macflim
{

struct frame
{
    frame(size_t W, size_t H) : source{std::in_place, W, H}, result{std::in_place, W, H} {}

    std::optional<bitmap> source; //  What we wanted to draw

    size_t ticks = 0;           //  Number of ticks grayscale is displayed
    std::vector<uint8_t> video; //  Encoded video delta
    std::vector<uint8_t> audio; //  Encoded audio

    std::optional<bitmap> result; //  What we actually drew

    //  #### passing silent is inelegant: we should not generate audio data when silenced
    size_t get_size(bool silent)
    {
        return video.size() + silent * audio.size();
    }

    frame(const bitmap &s, const size_t t, const std::vector<uint8_t> &v, const std::vector<uint8_t> &a,
          const bitmap &r)
        : source{s}, ticks{t}, video{v}, audio{a}, result{r}
    {
    }

    //  --- Binary serialization (movie component format) ---

    //  Serialize this frame's binary representation into out
    void serialize(std::vector<uint8_t> &out) const
    {
        auto o = std::back_inserter(out);
        write2(o, ticks);

        if (audio.empty())
            write2(o, 2); //  empty sound block
        else
        {
            write2(o, ticks * 370 + 8); //  sound block size
            write2(o, 0);               //  ffMode
            write4(o, 65536);           //  rate
            for (auto v : audio)
                write1(o, v);
        }

        write2(o, video.size() + 2); //  video block size (includes size field)
        for (auto v : video)
            write1(o, v);
    }

    //  Deserialize a frame from raw bytes (ticks, audio, video only — no framebuffers)
    static frame deserialize(const uint8_t *data, size_t size)
    {
        frame f;
        if (size < 4)
            return f;

        const uint8_t *p = data;

        f.ticks = read2(p);

        uint16_t sound_size = read2(p);
        if (sound_size > 2 && (p + sound_size - 2) <= (data + size))
        {
            /*uint16_t ff_mode =*/read2(p); //  ffMode (skip)
            /*uint32_t rate =*/read4(p);    //  rate (skip)
            size_t audio_size = sound_size - 8;
            f.audio.assign(p, p + audio_size);
            p += audio_size;
        }

        if (p + 2 <= data + size)
        {
            uint16_t video_size_field = read2(p);
            size_t video_size = video_size_field > 2 ? video_size_field - 2 : 0;
            if (video_size > 0 && p + video_size <= data + size)
            {
                f.video.assign(p, p + video_size);
                p += video_size;
            }
        }

        return f;
    }

  private:
    //  Private default constructor for deserialize()
    frame() = default;
};

} // namespace macflim
