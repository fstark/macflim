#include "ffmpeg_reader.hpp"

extern "C"
{
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
#include <libavcodec/avcodec.h>
}

#include <array>
#include <format>

extern bool sDebug;

/// This stores a sound buffer and transform it into a suitable format for flims
class sound_buffer
{
    std::vector<float> data_;
    size_t channel_count_ = 0; //  # of channels
    size_t sample_rate_ = 0;   //  # of samples per second
    float min_sample_;
    float max_sample_;

public:
    sound_buffer(size_t channel_count, size_t sample_rate) : channel_count_{channel_count},
                                                             sample_rate_{sample_rate}
    {
    }

    void append_silence(float duration)
    {
        size_t sample_count = sample_rate_ * duration;
        for (size_t i = 0; i != sample_count; i++)
        {
            data_.push_back(0);
        }
    }

    void append_samples(float **samples, size_t sample_count)
    {
        for (size_t i = 0; i != sample_count; i++)
        {
            float v = 0;
            for (size_t j = 0; j != channel_count_; j++)
            {
                v += samples[j][i];
            }
            data_.push_back(v / channel_count_);
        }
    }

    void process()
    {
        min_sample_ = *std::min_element(std::begin(data_), std::end(data_));
        max_sample_ = *std::max_element(std::begin(data_), std::end(data_));
    }

    std::unique_ptr<sound_frame_t> extract(size_t frame)
    {
        double t = frame / 60.0; //  Time in seconds
        size_t start = t * sample_rate_;

        if (start >= data_.size())
        {
            return nullptr;
        }

        auto fr = std::make_unique<sound_frame_t>();

        for (int i = 0; i != sound_frame_t::size; i++)
        {
            size_t index = start + (i / 370.0 / 60.0) * sample_rate_;
            if (index < data_.size())
            {
                fr->at(i) = (data_[index] - min_sample_) / (max_sample_ - min_sample_) * 255;
            }
            else
            {
                fr->at(i) = 128;
            }
        }

        return fr;
    }
};

/// Lazy video-only reader: decodes one video frame per next() call.
/// Audio is handled separately via the decode_audio() free function.
class ffmpeg_reader : public input_reader
{
    AVFormatContext *format_context_ = nullptr;
    const AVCodec *video_decoder_;
    AVStream *video_stream_;
    AVCodecContext *video_codec_context_ = nullptr;
    uint8_t *video_dst_data_[4] = {NULL};
    int video_dst_linesize_[4];
    AVPacket *pkt_;
    AVFrame *frame_;
    int ixv; //  Video stream index
    size_t video_frame_count_ = 0;
    std::unique_ptr<image> video_image_;   //  Size of the video input
    std::unique_ptr<image> default_image_; //  Size of our output
    double first_frame_second_;
    size_t frame_to_extract_;
    bool done_ = false; //  No more frames to decode

    /// Try to receive one decoded video frame.  Returns true if a frame was produced.
    bool receive_video_frame(image &out)
    {
        for (;;)
        {
            int ret = avcodec_receive_frame(video_codec_context_, frame_);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                return false;
            if (ret < 0)
            {
                char errbuf[AV_ERROR_MAX_STRING_SIZE];
                av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
                std::cerr << "Error during video decoding: " << errbuf << std::endl;
                return false;
            }

            // Skip frames before the requested start time
            if (frame_->pts * av_q2d(video_stream_->time_base) < first_frame_second_)
                continue;

            av_image_copy(
                video_dst_data_, video_dst_linesize_,
                (const uint8_t **)(frame_->data), frame_->linesize,
                video_codec_context_->pix_fmt, video_codec_context_->width,
                video_codec_context_->height);

            video_image_->set_luma(video_dst_data_[0]);

            out = *default_image_;
            copy(out, *video_image_, true, 0.5, 0.5);
            video_frame_count_++;
            //            std::clog << "Read " << video_frame_count_ << " frames\r" << std::flush;
            return true;
        }
    }

    void init_video_context()
    {
        video_codec_context_ = avcodec_alloc_context3(video_decoder_);
        if (!video_codec_context_)
            throw "CANNOT ALLOCATE VIDEO CODEC CONTEXT";

        if (avcodec_parameters_to_context(video_codec_context_, video_stream_->codecpar) < 0)
            throw "FAILED TO COPY VIDEO CODEC PARAMETERS";

        AVDictionary *opts = NULL;
        av_dict_set(&opts, "refcounted_frames", "0", 0); //  Do not refcount

        if (avcodec_open2(video_codec_context_, video_decoder_, &opts) < 0)
            throw "CANNOT OPEN VIDEO CODEC";

        if (sDebug)
        {
            std::clog << "VIDEO CODEC OPENED WITH PIXEL FORMAT "
                      << av_get_pix_fmt_name(video_codec_context_->pix_fmt) << "\n";
        }

        if (video_codec_context_->pix_fmt != AV_PIX_FMT_YUV420P)
            throw "WAS EXPECTING A YUV420P PIXEL FORMAT";
    }

public:
    ffmpeg_reader(const std::string &movie_path, timestamp_t from, timestamp_t duration)
    {
        av_log_set_level(AV_LOG_WARNING);

        if (avformat_open_input(&format_context_, movie_path.c_str(), NULL, NULL) != 0)
            throw "Cannot open input file";

        if (avformat_find_stream_info(format_context_, NULL) < 0)
            throw "Cannot find stream information";

        if (sDebug)
        {
            std::clog << "Searching for video in " << format_context_->nb_streams << " streams\n";
        }

        ixv = av_find_best_stream(format_context_, AVMEDIA_TYPE_VIDEO, -1, -1, &video_decoder_, 0);

        if (ixv == AVERROR_STREAM_NOT_FOUND)
            throw "NO VIDEO IN FILE";

        if (ixv == AVERROR_DECODER_NOT_FOUND)
            throw "NO SUITABLE VIDEO DECODER AVAILABLE";

        if (sDebug)
        {
            std::clog << "Video stream index :" << ixv << "\n";
        }

        timestamp_t actual_duration = format_context_->duration / (double)AV_TIME_BASE;
        if (duration > actual_duration)
        {
            std::clog << "Warning: Requested duration (" << duration << "s) exceeds video length ("
                      << actual_duration << "s). Trimming to video length.\n";
            duration = actual_duration;
        }

        timestamp_t seek_to = std::max(from - 10.0, 0.0); //  We seek to 10 seconds earlier, if we can
        if (avformat_seek_file(format_context_, -1, seek_to * AV_TIME_BASE,
                               seek_to * AV_TIME_BASE, seek_to * AV_TIME_BASE,
                               AVSEEK_FLAG_ANY) < 0)
            throw "CANNOT SEEK IN FILE";

        video_stream_ = format_context_->streams[ixv];

        if (sDebug)
        {
            std::clog << "Video : " << video_stream_->codecpar->width << "x"
                      << video_stream_->codecpar->height << "@"
                      << av_q2d(video_stream_->r_frame_rate) << " fps"
                      << " timebase:" << av_q2d(video_stream_->time_base) << "\n";
        }

        first_frame_second_ = from;
        frame_to_extract_ = duration * av_q2d(video_stream_->r_frame_rate);

        init_video_context();

        auto bufsize = av_image_alloc(
            video_dst_data_,
            video_dst_linesize_,
            video_codec_context_->width,
            video_codec_context_->height,
            video_codec_context_->pix_fmt,
            1);

        if (bufsize < 0)
            throw "CANNOT ALLOCATE IMAGE";

        video_image_ = std::make_unique<image>(video_codec_context_->width, video_codec_context_->height);

        // #### need to clarify what size we want when extracting. Why the hard-coded 512x342?
        double aspect = video_codec_context_->width / (double)video_codec_context_->height;
        if (aspect > 512 / 342.0)
            default_image_ = std::make_unique<image>(342 * aspect, 342);
        else
            default_image_ = std::make_unique<image>(512, 512 / aspect);

        if (sDebug)
        {
            std::clog << "Image structure:\n";
            std::clog << video_dst_linesize_[0] << " " << video_dst_linesize_[1] << " "
                      << video_dst_linesize_[2] << " " << video_dst_linesize_[3] << "\n";
            std::clog << (video_dst_linesize_[0] + video_dst_linesize_[1] +
                          video_dst_linesize_[2] + video_dst_linesize_[3]) *
                             video_codec_context_->height
                      << "\n";
            std::clog << bufsize << "\n";
        }

        frame_ = av_frame_alloc();

        pkt_ = av_packet_alloc();
        if (!pkt_)
            throw "Failed to allocate packet";
    }

    ~ffmpeg_reader()
    {
        avformat_close_input(&format_context_);
        if (video_codec_context_)
        {
            avcodec_free_context(&video_codec_context_);
        }
        av_frame_free(&frame_);
        av_packet_free(&pkt_);
        av_freep(&video_dst_data_[0]);
        if (sDebug)
        {
            std::clog << "Closed media file\n";
        }
    }

    virtual double frame_rate()
    {
        return av_q2d(video_stream_->r_frame_rate);
    }

    /// Lazily decode and return the next video frame, or nullptr when done.
    virtual std::unique_ptr<image> next()
    {
        if (done_)
            return nullptr;

        if (video_frame_count_ >= frame_to_extract_)
        {
            done_ = true;
            return nullptr;
        }

        auto result = std::make_unique<image>(default_image_->W(), default_image_->H());

        // Try to get a frame from already-sent packets
        if (receive_video_frame(*result))
            return result;

        // Read more packets until we get a video frame
        while (av_read_frame(format_context_, pkt_) >= 0)
        {
            if (pkt_->stream_index == ixv)
            {
                int ret = avcodec_send_packet(video_codec_context_, pkt_);
                av_packet_unref(pkt_);
                if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF)
                {
                    char errbuf[AV_ERROR_MAX_STRING_SIZE];
                    av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
                    std::cerr << "Error sending video packet: " << errbuf << std::endl;
                    done_ = true;
                    return nullptr;
                }

                if (receive_video_frame(*result))
                {
                    if (video_frame_count_ >= frame_to_extract_)
                        done_ = true;
                    return result;
                }
            }
            else
            {
                av_packet_unref(pkt_); // Skip non-video packets
            }
        }

        // No more packets — flush the decoder
        avcodec_send_packet(video_codec_context_, nullptr);
        if (receive_video_frame(*result))
            return result;

        done_ = true;
        return nullptr;
    }

    /// Audio is handled by the separate decode_audio() function
    virtual std::unique_ptr<sound_frame_t> next_sound()
    {
        return nullptr;
    }
};

std::unique_ptr<input_reader> make_ffmpeg_reader(const std::string &movie_path, timestamp_t from, timestamp_t duration)
{
    try
    {
        return std::make_unique<ffmpeg_reader>(movie_path, from, duration);
    }
    catch (const char *e)
    {
        std::clog << "**** ERROR : " << e << "\n";
        return nullptr;
    }
}

/// Decode only the audio stream from a media file, normalize, and convert to Mac sound frames.
std::vector<sound_frame_t> decode_audio(const std::string &movie_path, timestamp_t from, timestamp_t duration)
{
    std::vector<sound_frame_t> result;

    AVFormatContext *format_context = nullptr;
    AVCodecContext *audio_codec_context = nullptr;
    AVFrame *frame = nullptr;
    AVPacket *pkt = nullptr;

    try
    {
        av_log_set_level(AV_LOG_WARNING);

        if (avformat_open_input(&format_context, movie_path.c_str(), NULL, NULL) != 0)
            throw "Cannot open input file for audio";

        assert(format_context);

        if (avformat_find_stream_info(format_context, NULL) < 0)
            throw "Cannot find stream information for audio";

        const AVCodec *audio_decoder = nullptr;
        int ixa = av_find_best_stream(format_context, AVMEDIA_TYPE_AUDIO, -1, -1, &audio_decoder, 0);

        if (ixa == AVERROR_STREAM_NOT_FOUND)
        {
            std::cerr << "NO SOUND -- INSERTING SILENCE\n";
            avformat_close_input(&format_context);
            return result;
        }

        if (ixa == AVERROR_DECODER_NOT_FOUND)
            throw "NO SUITABLE AUDIO DECODER AVAILABLE";

        AVStream *audio_stream = format_context->streams[ixa];

        audio_codec_context = avcodec_alloc_context3(audio_decoder);
        if (!audio_codec_context)
            throw "CANNOT ALLOCATE AUDIO CODEC CONTEXT";

        if (avcodec_parameters_to_context(audio_codec_context, audio_stream->codecpar) < 0)
            throw "FAILED TO COPY AUDIO CODEC PARAMETERS";

        if (avcodec_open2(audio_codec_context, audio_decoder, nullptr) < 0)
            throw "CANNOT OPEN AUDIO CODEC";

        if (sDebug)
        {
            std::clog << "AUDIO CODEC: " << avcodec_get_name(audio_codec_context->codec_id) << "\n";
            AVSampleFormat sfmt = audio_codec_context->sample_fmt;
            int n_channels = audio_codec_context->ch_layout.nb_channels;
            std::clog << "SAMPLE FORMAT:" << av_get_sample_fmt_name(sfmt) << "\n";
            std::clog << "# CHANNELS   :" << n_channels << "\n";
            std::clog << "SAMPLE RATE  :" << audio_codec_context->sample_rate << "\n";
        }

        int n_channels = audio_codec_context->ch_layout.nb_channels;
        sound_buffer sound(n_channels, audio_codec_context->sample_rate);

        std::clog << std::format("Audio stream: {}Hz, {} channel(s)\n", 
                                 audio_codec_context->sample_rate, n_channels);

        // Seek to just before the requested start
        timestamp_t seek_to = std::max(from - 10.0, 0.0);
        std::clog << std::format("Seeking to {:.1f}s (target: {:.1f}s, duration: {:.1f}s, end: {:.1f}s)\n",
                                 seek_to, from, duration, from + duration);
        if (avformat_seek_file(format_context, -1, seek_to * AV_TIME_BASE,
                               seek_to * AV_TIME_BASE, seek_to * AV_TIME_BASE,
                               AVSEEK_FLAG_ANY) < 0)
            throw "CANNOT SEEK IN FILE FOR AUDIO";

        frame = av_frame_alloc();
        pkt = av_packet_alloc();
        if (!pkt)
            throw "Failed to allocate packet for audio";

        bool found_sound = false;
        double end_time = from + duration; // Stop decoding audio after this time
        double last_log_time = from;

        // Pass 1: decode all audio packets into the sound_buffer
        std::clog << "Decoding audio...\n";
        while (av_read_frame(format_context, pkt) >= 0)
        {
            if (pkt->stream_index == ixa)
            {
                int ret = avcodec_send_packet(audio_codec_context, pkt);
                av_packet_unref(pkt);
                if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF)
                    continue;

                while (true)
                {
                    ret = avcodec_receive_frame(audio_codec_context, frame);
                    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                        break;
                    if (ret < 0)
                        break;

                    double pts = frame->pts * av_q2d(audio_stream->time_base);
                    if (pts >= from && pts < end_time)
                    {
                        // Log progress every 5 seconds
                        if (pts - last_log_time >= 5.0)
                        {
                            double progress = (pts - from) / duration * 100.0;
                            std::clog << std::format("  Decoding audio: {:.1f}s / {:.1f}s ({:.0f}%)\r",
                                                     pts, end_time, progress);
                            std::clog.flush();
                            last_log_time = pts;
                        }

                        if (!found_sound)
                        {
                            found_sound = true;
                            auto skip = pts - from;
                            if (skip > 0)
                            {
                                std::clog << std::format("Inserting {:.3f} seconds of silence\n", skip);
                                sound.append_silence(skip);
                            }
                        }
                        sound.append_samples((float **)frame->extended_data, frame->nb_samples);
                    }
                    else if (pts >= end_time)
                    {
                        // We've passed the end time, no need to continue
                        goto done_decoding;
                    }
                }
            }
            else
            {
                av_packet_unref(pkt);
            }
        }
        done_decoding:

        // Flush the audio decoder to get any remaining frames
        avcodec_send_packet(audio_codec_context, nullptr);
        while (true)
        {
            int ret = avcodec_receive_frame(audio_codec_context, frame);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                break;
            if (ret < 0)
                break;
            double pts = frame->pts * av_q2d(audio_stream->time_base);
            if (pts >= from && pts < end_time)
            {
                if (!found_sound)
                {
                    found_sound = true;
                    auto skip = pts - from;
                    if (skip > 0)
                        sound.append_silence(skip);
                }
                sound.append_samples((float **)frame->extended_data, frame->nb_samples);
            }
            else if (pts >= end_time)
            {
                break; // Stop flushing once we've passed end_time
            }
        }

        // Normalize (find min/max)
        sound.process();

        // Convert to Mac sound frames (370 bytes each at 1/60th second)
        std::clog << "\nConverting audio to Mac format...\n";
        for (size_t i = 0;; i++)
        {
            auto sf = sound.extract(i);
            if (!sf)
                break;
            result.push_back(*sf);
        }

        std::clog << std::format("Audio: {} sound frames ({:.2f}s)\n", 
                                 result.size(), result.size() / 60.0);

        // Clean up
        av_frame_free(&frame);
        av_packet_free(&pkt);
        avcodec_free_context(&audio_codec_context);
        avformat_close_input(&format_context);
    }
    catch (...)
    {
        if (frame)
            av_frame_free(&frame);
        if (pkt)
            av_packet_free(&pkt);
        if (audio_codec_context)
            avcodec_free_context(&audio_codec_context);
        if (format_context)
            avformat_close_input(&format_context);
        throw;
    }

    return result;
}
