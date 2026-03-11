#include "flim_source.hpp"

#include "../decoder.hpp"
#include "../file_handle.hpp"
#include "../flim.hpp"
#include "../frame.hpp"
#include "../imgcompress.hpp"

#include <format>
#include <memory>
#include <stdexcept>
#include <vector>

namespace macflim
{

namespace
{

/// Extract the initial screen from a flim, defaulting to all-black (0xFF).
bitmap extract_initial(const flim &fl, uint16_t width, uint16_t height)
{
    auto *data = fl.find_component_data(component_type::initial);
    if (data && data->size() > 6)
    {
        const uint8_t *p = data->data();
        /*uint16_t type =*/read2(p);
        uint16_t w = read2(p);
        uint16_t h = read2(p);
        std::vector<uint8_t> bytes(p, p + (data->size() - 6));
        return bitmap(bytes, w, h, false);
    }

    //  Encoder assumes all-black (0xFF) starting state
    bitmap blank(width, height);
    blank.fill(0xFF);
    return blank;
}

/// Parse the TOC into frame {offset, size} pairs.
struct frame_loc
{
    size_t offset;
    size_t size;
};

std::vector<frame_loc> parse_toc(const std::vector<uint8_t> &toc_data)
{
    std::vector<frame_loc> locs;
    const uint8_t *p = toc_data.data();
    size_t offset = 0;
    size_t count = toc_data.size() / 2;
    locs.reserve(count);

    for (size_t i = 0; i < count; ++i)
    {
        uint16_t frame_size = read2(p);
        locs.push_back({offset, frame_size});
        offset += frame_size;
    }
    return locs;
}

/// Decode all .flim frames into target bitmaps by replaying deltas sequentially.
std::vector<bitmap> decode_all_frames(const flim &fl, const flim_info &info)
{
    auto *toc_data = fl.find_component_data(component_type::toc);
    auto *movie_data = fl.find_component_data(component_type::movie);
    if (!toc_data || !movie_data)
        throw std::runtime_error("Flim file missing TOC or movie component");

    auto locs = parse_toc(*toc_data);
    bitmap screen = extract_initial(fl, info.width, info.height);

    std::vector<bitmap> targets;
    targets.reserve(locs.size());

    for (auto &loc : locs)
    {
        frame f = frame::deserialize(movie_data->data() + loc.offset, loc.size);
        if (!f.video.empty())
            apply_delta(screen, f.video);
        targets.push_back(screen);
    }

    return targets;
}

} // namespace

frame_source make_flim_source(std::string_view path)
{
    file_handle fh(path, "rb");
    flim fl;
    fl.read(fh);

    auto *info_data = fl.find_component_data(component_type::info);
    if (!info_data)
        throw std::runtime_error("Flim file has no info component");

    flim_info info;
    info.deserialize(info_data->data(), info_data->size());

    //  Decode all frames upfront into shared storage
    auto frames = std::make_shared<std::vector<bitmap>>(decode_all_frames(fl, info));
    auto index = std::make_shared<size_t>(0);

    return [frames, index]() -> std::optional<bitmap>
    {
        if (*index >= frames->size())
            return std::nullopt;
        return (*frames)[(*index)++];
    };
}

} // namespace macflim
