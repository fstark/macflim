#include "filesystem_reader.hpp"

/*
 * A filesystem reader can read 512x342 8 bits pgm files numbered from 1. Audio has to be raw 8 bits unsigned.
 */
class filesystem_reader : public input_reader
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
    filesystem_reader(const std::string &file_pattern, double frame_rate, const std::string &audio_path, size_t from_frame, size_t count) : file_pattern_{file_pattern},
                                                                                                                                            frame_rate_{frame_rate},
                                                                                                                                            audio_path_{audio_path},
                                                                                                                                            from_frame_{from_frame},
                                                                                                                                            count_{count}
    {
        current_image_index_ = from_frame_;
    }

    virtual double frame_rate() { return frame_rate_; }

    virtual std::unique_ptr<image> next()
    {
        auto img = std::make_unique<image>(512, 342); //  'cause read_image don't support anything else for now

        if (image_read_)
            return nullptr;

        if (current_image_index_ >= from_frame_ + count_)
        {
            image_read_ = true;
            return nullptr;
        }

        char buffer[1024];
        sprintf(buffer, file_pattern_.c_str(), current_image_index_);

        if (!read_image(*(img.get()), buffer))
        {
            image_read_ = true;
            return nullptr;
        }

        current_image_index_++;

        std::clog << "." << std::flush;

        return img;
    }

    /*
        virtual size_t sample_rate() { return 370*60; };

        virtual std::vector<double> raw_sound()
        {
            std::vector<uint8_t> audio_samples;

            long audio_start = frame_from_image(from_frame_)*370;    //  Images are one-based
            long audio_end = frame_from_image(from_frame_+count_)*370;      //  Last image is included?
            long audio_size = audio_end-audio_start;

            FILE *f = fopen( audio_path_.c_str(), "rb" );
            if (f)
            {
                audio_samples.resize( audio_size );
                for (auto &v:audio_samples)
                    v = 0x80;

                fseek( f, audio_start, SEEK_SET );
                auto res = fread( audio_samples.data(), 1, audio_size, f );
                if (res!=audio_size)
                    std::clog << "AUDIO: added " << audio_size-res << " bytes of silence\n";
                fclose( f );
            }
            else
                std::cerr << "**** ERROR: CANNOT OPEN AUDIO FILE [" << audio_path_ << "]\n";
            std::clog << "AUDIO: READ " << audio_size << " bytes from offset " << audio_start << "\n";

            auto min_sample = *std::min_element( std::begin(audio_samples), std::end(audio_samples) );
            auto max_sample = *std::max_element( std::begin(audio_samples), std::end(audio_samples) );
            std::clog << " SAMPLE MIN:" << (int)min_sample << " SAMPLE MAX:" << (int)max_sample << "\n";

            std::vector<double> res;
            for (auto s:audio_samples)
                res.push_back( (s-128.0)/128.0 );

            return res;
        }
    */
    virtual std::unique_ptr<sound_frame_t> next_sound() { return nullptr; } //  #### THIS IS COMPLETELY WRONG
};

std::unique_ptr<input_reader> make_filesystem_reader(
	std::string &input_file,
	double fps,
	std::string &audio_arg,
	double from_index,	//	bad name, is a timestamp
	double to_index
)
{
	return std::make_unique<filesystem_reader>(input_file, fps, audio_arg, from_index, to_index);
}

