#pragma once

//  Flim file format types, reading, and writing

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "frame.hpp"
#include "imgcompress.hpp"

namespace macflim
{

//  --- Component types in a flim file ---

enum class component_type : uint16_t
{
    info = 0,
    movie = 1,
    toc = 2,
    poster = 3,
    initial = 4
};

//  --- Component directory entry (on-disk layout) ---

struct component_entry
{
    uint16_t type;
    uint32_t offset;
    uint32_t size;

    const char *type_name() const
    {
        static constexpr const char *names[] = {"info", "movie", "toc", "poster", "initial"};
        return type < std::size(names) ? names[type] : "unknown";
    }
};

//  --- The info component of a flim, as stored on disk ---

struct flim_info
{
    size_t width_;       //  2 bytes
    size_t height_;      //  2 bytes
    bool silent_;        //  2 bytes
    size_t frame_count_; //  4 bytes
    size_t total_ticks_; //  4 bytes
    size_t byterate_;    //  2 bytes

    flim_info() : width_(0), height_(0), silent_(false), frame_count_(0), total_ticks_(0), byterate_(0) {}

    flim_info(size_t width, size_t height, bool silent, size_t frame_count, size_t total_ticks, size_t byterate)
        : width_(width), height_(height), silent_(silent), frame_count_(frame_count), total_ticks_(total_ticks),
          byterate_(byterate)
    {
    }

    void serialize(std::vector<uint8_t> &out) const
    {
        auto o = std::back_inserter(out);
        write2(o, width_);
        write2(o, height_);
        write2(o, silent_ ? 1 : 0);
        write4(o, frame_count_);
        write4(o, total_ticks_);
        write2(o, byterate_);
    }

    void deserialize(const uint8_t *data, size_t size)
    {
        if (size < 16)
            return;
        const uint8_t *p = data;
        width_ = read2(p);
        height_ = read2(p);
        silent_ = read2(p) != 0;
        frame_count_ = read4(p);
        total_ticks_ = read4(p);
        byterate_ = read2(p);
    }
};

//  --- Fletcher checksum ---

void fletcher(long &checksum, const std::vector<uint8_t> &data);
void fletcher(long &checksum, uint16_t data);

//  --- A flim: a directory of typed components with binary data ---
//
//  Populate by reading from disk (read) or building in memory
//  (add_component, add, etc.), then write to disk (write).

class flim
{
    static constexpr size_t COMMENT_SIZE = 1022;
    static constexpr size_t CHECKSUM_SIZE = 2;

    std::string comment_;
    uint16_t version_ = 1;
    std::vector<component_entry> components_;
    std::vector<std::vector<uint8_t>> blobs_;

    //  Serialize the header directory (version + component table)
    std::vector<uint8_t> serialize_header() const;

    //  Compute the Fletcher-16 checksum over header + all blobs
    uint16_t compute_checksum() const;

    //  Write helpers
    static void write_u16(FILE *f, uint16_t v);
    static void write_bytes(FILE *f, const std::vector<uint8_t> &v);

  public:
    //  Construct an empty flim for building in memory
    flim() = default;

    //  Construct a flim with a comment (for encoding)
    explicit flim(const std::string &comment);

    //  --- Disk I/O ---

    bool read(FILE *f);
    void write(FILE *f) const;

    //  --- Accessors ---

    const std::string &comment() const
    {
        return comment_;
    }
    uint16_t version() const
    {
        return version_;
    }
    size_t component_count() const
    {
        return components_.size();
    }
    const component_entry &component(size_t i) const
    {
        return components_[i];
    }
    const std::vector<uint8_t> &component_data(size_t i) const
    {
        return blobs_[i];
    }

    //  Find first component of a given type, returns nullptr if not found
    const std::vector<uint8_t> *find_component_data(component_type type) const
    {
        for (size_t i = 0; i < components_.size(); i++)
            if (components_[i].type == static_cast<uint16_t>(type))
                return &blobs_[i];
        return nullptr;
    }

    //  --- Building ---

    void add_component(component_type type, const std::vector<uint8_t> &data);

    //  Adds the flim info component
    void add(const flim_info &fi);

    //  Adds all the frames and generates the movie and toc components
    void add(const std::vector<frame> &frames);

    void add_framebuffer(component_type type, const bitmap &fb);
    void add_poster(const bitmap &fb);
    void add_initial(const bitmap &fb);
};

} // namespace macflim
