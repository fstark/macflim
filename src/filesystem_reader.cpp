#include "filesystem_reader.hpp"

#include "common.hpp"

namespace macflim
{

/*
 * A filesystem reader can read 512x342 8 bits pgm files numbered from 1. Audio has to be raw 8 bits unsigned.
 */
class filesystem_reader final : public input_reader
{
    std::string file_pattern_;
    double frame_rate_;
    std::string audio_path_;
    size_t from_frame_;
    size_t count_;

    size_t current_image_index_;
    bool image_read_ = false;

    size_t frame_from_image(size_t n) const
    {
        return ticks_from_frame(n - 1, frame_rate_);
    }

  public:
    filesystem_reader(const std::string &file_pattern, double frame_rate, const std::string &audio_path,
                      size_t from_frame, size_t count)
        : file_pattern_{file_pattern}, frame_rate_{frame_rate}, audio_path_{audio_path}, from_frame_{from_frame},
          count_{count}
    {
        current_image_index_ = from_frame_;
    }

    double frame_rate() override
    {
        return frame_rate_;
    }

    std::unique_ptr<grayscale> next() override
    {
        auto img = std::make_unique<grayscale>(512, 342); //  'cause read_grayscale don't support anything else for now

        if (image_read_)
            return nullptr;

        if (current_image_index_ >= from_frame_ + count_)
        {
            image_read_ = true;
            return nullptr;
        }

        std::string buffer = simplesprintf(file_pattern_.c_str(), current_image_index_);

        if (!read_grayscale(*(img.get()), buffer.c_str()))
        {
            image_read_ = true;
            return nullptr;
        }

        current_image_index_++;

        std::clog << "." << std::flush;

        return img;
    }

    std::unique_ptr<sound_frame_t> next_sound() override
    {
        return nullptr;
    } //  #### THIS IS COMPLETELY WRONG
};

std::unique_ptr<input_reader> make_filesystem_reader(std::string &input_file, double fps, std::string &audio_arg,
                                                     size_t from_index, //	bad name, is a timestamp
                                                     size_t to_index)
{
    return std::make_unique<filesystem_reader>(input_file, fps, audio_arg, from_index, to_index);
}
} // namespace macflim