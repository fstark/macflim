#include "reader.hpp"

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
#include <libavcodec/avcodec.h>
}

#include <array>
#include <iostream>
#include <stdexcept>
#include <cstring>
#include <algorithm>

extern bool sDebug;

class sound_buffer
{
    std::vector<float> data_;
    size_t channel_count_ = 0;
    size_t sample_rate_ = 0;
    float min_sample_;
    float max_sample_;

public:
    sound_buffer(size_t channel_count, size_t sample_rate) :
        channel_count_{channel_count},
        sample_rate_{sample_rate}
    {}

    void append_silence(float duration)
    {
        size_t sample_count = sample_rate_ * duration;
        data_.insert(data_.end(), sample_count, 0.0f);
    }

    void append_samples(float **samples, size_t sample_count)
    {
        for (size_t i = 0; i < sample_count; i++)
        {
            float v = 0;
            for (size_t j = 0; j < channel_count_; j++)
                v += samples[j][i];
            data_.push_back(v / channel_count_);
        }
    }

    void process()
    {
        if (data_.empty()) {
            min_sample_ = max_sample_ = 0.0f;
            return;
        }
        min_sample_ = *std::min_element(data_.begin(), data_.end());
        max_sample_ = *std::max_element(data_.begin(), data_.end());
    }

    std::unique_ptr<sound_frame_t> extract(size_t frame)
    {
        double t = frame / 60.0;
        size_t start = t * sample_rate_;

        if (start >= data_.size())
            return nullptr;

        auto fr = std::make_unique<sound_frame_t>();

        for (size_t i = 0; i < sound_frame_t::size; i++)
        {
            size_t index = start + (i / 370.0 / 60.0) * sample_rate_;
            if (index < data_.size())
                fr->at(i) = (data_[index] - min_sample_) / (max_sample_ - min_sample_) * 255;
            else
                fr->at(i) = 128;
        }

        return fr;
    }
};

class ffmpeg_reader : public input_reader
{
    AVFormatContext *format_context_ = nullptr;
    const AVCodec *video_decoder_ = nullptr;
    const AVCodec *audio_decoder_ = nullptr;
    AVStream *video_stream_ = nullptr;
    AVStream *audio_stream_ = nullptr;
    AVCodecContext *video_codec_context_ = nullptr;
    AVCodecContext *audio_codec_context_ = nullptr;
    uint8_t *video_dst_data_[4] = {NULL};
    int video_dst_linesize_[4];
    AVPacket *pkt_ = nullptr;
    AVFrame *frame_ = nullptr;
    int ixv = -1;
    int ixa = -1;
    size_t video_frame_count = 0;
    std::unique_ptr<image> video_image_;
    std::unique_ptr<image> default_image_;
    std::vector<image> images_;
    std::unique_ptr<sound_buffer> sound_;
    int image_ix = -1;
    int sound_ix = -1;
    double first_frame_second_;
    size_t frame_to_extract_;
    bool found_sound_ = false;

    int decode_packet(int *got_frame, AVPacket *pkt)
    {
        int ret = 0;
        *got_frame = 0;

        if (pkt->stream_index == ixv)
        {
            ret = avcodec_send_packet(video_codec_context_, pkt);
            if (ret < 0) {
                char errbuf[AV_ERROR_MAX_STRING_SIZE];
                av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
                std::cerr << "Error sending video packet for decoding: " << errbuf << std::endl;
                return ret;
            }

            while (ret >= 0)
            {
                ret = avcodec_receive_frame(video_codec_context_, frame_);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                {
                    break;
                }
                else if (ret < 0)
                {
                    char errbuf[AV_ERROR_MAX_STRING_SIZE];
                    av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
                    std::cerr << "Error receiving decoded video frame: " << errbuf << std::endl;
                    return ret;
                }

                *got_frame = 1;

                double frame_pts = frame_->pts * av_q2d(video_stream_->time_base);
                if (frame_pts >= first_frame_second_ && images_.size() < frame_to_extract_)
                {
                    video_frame_count++;
                    std::clog << "Read " << video_frame_count << " frames, PTS: " << frame_pts << "\r" << std::flush;

                    av_image_copy(
                        video_dst_data_, video_dst_linesize_,
                        (const uint8_t **)(frame_->data), frame_->linesize,
                        video_codec_context_->pix_fmt, video_codec_context_->width, video_codec_context_->height);
                    
                    video_image_->set_luma(video_dst_data_[0]);
                    images_.push_back(*default_image_);
                    copy(images_.back(), *video_image_);

                    if (sDebug)
                        std::clog << "Copied frame " << video_frame_count << " to images_\n";
                }

                av_frame_unref(frame_);
            }
        }
        else if (pkt->stream_index == ixa)
        {
            ret = avcodec_send_packet(audio_codec_context_, pkt);
            if (ret < 0) {
                char errbuf[AV_ERROR_MAX_STRING_SIZE];
                av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
                std::cerr << "Error sending audio packet for decoding: " << errbuf << std::endl;
                return ret;
            }

            while (ret >= 0)
            {
                ret = avcodec_receive_frame(audio_codec_context_, frame_);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                {
                    break;
                }
                else if (ret < 0)
                {
                    char errbuf[AV_ERROR_MAX_STRING_SIZE];
                    av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
                    std::cerr << "Error receiving decoded audio frame: " << errbuf << std::endl;
                    return ret;
                }

                *got_frame = 1;

                double frame_pts = frame_->pts * av_q2d(audio_stream_->time_base);
                if (frame_pts >= first_frame_second_)
                {
                    if (!found_sound_)
                    {
                        found_sound_ = true;
                        auto skip = frame_pts - first_frame_second_;
                        if (skip > 0)
                        {
                            std::clog << "Inserting " << skip << " seconds of silence\n";
                            sound_->append_silence(skip);
                        }
                    }
                    sound_->append_samples((float **)frame_->extended_data, frame_->nb_samples);
                }

                av_frame_unref(frame_);
            }
        }

        return 0;
    }

public:
    ffmpeg_reader() {}

    ffmpeg_reader(const std::string &movie_path, double from, double duration)
    {
        av_log_set_level(AV_LOG_WARNING);

        int ret = avformat_open_input(&format_context_, movie_path.c_str(), NULL, NULL);
        if (ret != 0) {
            char errbuf[AV_ERROR_MAX_STRING_SIZE];
            av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
            throw std::runtime_error(std::string("Cannot open input file: ") + errbuf);
        }

        ret = avformat_find_stream_info(format_context_, NULL);
        if (ret < 0) {
            char errbuf[AV_ERROR_MAX_STRING_SIZE];
            av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
            throw std::runtime_error(std::string("Cannot find stream information: ") + errbuf);
        }

        if (sDebug)
            std::clog << "Searching for audio and video in " << format_context_->nb_streams << " streams\n";

        ixv = av_find_best_stream(format_context_, AVMEDIA_TYPE_VIDEO, -1, -1, &video_decoder_, 0);
        ixa = av_find_best_stream(format_context_, AVMEDIA_TYPE_AUDIO, -1, -1, &audio_decoder_, 0);

        if (ixv == AVERROR_STREAM_NOT_FOUND) {
            throw std::runtime_error("NO VIDEO IN FILE");
        }
        if (ixv == AVERROR_DECODER_NOT_FOUND) {
            throw std::runtime_error("NO SUITABLE VIDEO DECODER AVAILABLE");
        }
        if (ixa == AVERROR_STREAM_NOT_FOUND) {
            std::cerr << "NO SOUND -- INSERTING SILENCE";
        }
        if (ixa == AVERROR_DECODER_NOT_FOUND) {
            throw std::runtime_error("NO SUITABLE AUDIO DECODER AVAILABLE");
        }

        if (sDebug)
        {
            std::clog << "Video stream index :" << ixv << "\n";
            std::clog << "Audio stream index :" << ixa << "\n";
        }

        double seek_to = std::max(from - 10.0, 0.0);
        ret = avformat_seek_file(format_context_, -1, seek_to * AV_TIME_BASE, seek_to * AV_TIME_BASE, seek_to * AV_TIME_BASE, AVSEEK_FLAG_ANY);
        if (ret < 0) {
            char errbuf[AV_ERROR_MAX_STRING_SIZE];
            av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
            throw std::runtime_error(std::string("CANNOT SEEK IN FILE: ") + errbuf);
        }

        video_stream_ = format_context_->streams[ixv];
        audio_stream_ = ixa != AVERROR_STREAM_NOT_FOUND ? format_context_->streams[ixa] : nullptr;

        if (sDebug)
        {
            std::clog << "Video : "
                      << " " << video_stream_->codecpar->width << "x" << video_stream_->codecpar->height
                      << "@" << av_q2d(video_stream_->r_frame_rate)
                      << " fps"
                      << " timebase:"
                      << av_q2d(video_stream_->time_base)
                      << "\n";
            if (audio_stream_)
            {
                std::clog << "Audio : "
                          << " " << audio_stream_->codecpar->sample_rate
                          << "Hz \n";
            }
        }

        first_frame_second_ = from;
        frame_to_extract_ = duration * av_q2d(video_stream_->r_frame_rate);

        video_codec_context_ = avcodec_alloc_context3(video_decoder_);
        if (!video_codec_context_)
            throw std::runtime_error("CANNOT ALLOCATE VIDEO CODEC CONTEXT");

        ret = avcodec_parameters_to_context(video_codec_context_, video_stream_->codecpar);
        if (ret < 0) {
            char errbuf[AV_ERROR_MAX_STRING_SIZE];
            av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
            throw std::runtime_error(std::string("FAILED TO COPY VIDEO CODEC PARAMETERS: ") + errbuf);
        }

        if (ixa != AVERROR_STREAM_NOT_FOUND)
        {
            audio_codec_context_ = avcodec_alloc_context3(audio_decoder_);
            if (!audio_codec_context_)
                throw std::runtime_error("CANNOT ALLOCATE AUDIO CODEC CONTEXT");

            ret = avcodec_parameters_to_context(audio_codec_context_, audio_stream_->codecpar);
            if (ret < 0) {
                char errbuf[AV_ERROR_MAX_STRING_SIZE];
                av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
                throw std::runtime_error(std::string("FAILED TO COPY AUDIO CODEC PARAMETERS: ") + errbuf);
            }
        }

        AVDictionary *opts = NULL;
        av_dict_set(&opts, "refcounted_frames", "0", 0);

        ret = avcodec_open2(video_codec_context_, video_decoder_, &opts);
        if (ret < 0) {
            char errbuf[AV_ERROR_MAX_STRING_SIZE];
            av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
            throw std::runtime_error(std::string("CANNOT OPEN VIDEO CODEC: ") + errbuf);
        }

        if (ixa != AVERROR_STREAM_NOT_FOUND)
        {
            ret = avcodec_open2(audio_codec_context_, audio_decoder_, nullptr);
            if (ret < 0) {
                char errbuf[AV_ERROR_MAX_STRING_SIZE];
                av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
                throw std::runtime_error(std::string("CANNOT OPEN AUDIO CODEC: ") + errbuf);
            }

            if (sDebug)
                std::clog << "AUDIO CODEC: " << avcodec_get_name(audio_codec_context_->codec_id) << "\n";

            AVSampleFormat sfmt = audio_codec_context_->sample_fmt;
            int n_channels = audio_codec_context_->ch_layout.nb_channels;

            if (sDebug)
            {
                std::clog << "SAMPLE FORMAT:" << av_get_sample_fmt_name(sfmt) << "\n";
                std::clog << "# CHANNELS   :" << n_channels << "\n";
                std::clog << "PLANAR       :" << (av_sample_fmt_is_planar(sfmt) ? "YES" : "NO") << "\n";
            }

            if (av_sample_fmt_is_planar(sfmt))
            {
                sfmt = av_get_packed_sample_fmt(sfmt);

                if (sDebug)
                {
                    std::clog << "PACKED FORMAT:" << av_get_sample_fmt_name(sfmt) << "\n";
                    std::clog << "SAMPLE RATE  :" << audio_codec_context_->sample_rate << "\n";
                }
            }
            sound_ = std::make_unique<sound_buffer>(n_channels, audio_codec_context_->sample_rate);
        }
        else
            sound_ = nullptr;

        if (sDebug)
            std::clog << "VIDEO CODEC OPENED WITH PIXEL FORMAT " << av_get_pix_fmt_name(video_codec_context_->pix_fmt) << "\n";

        if (video_codec_context_->pix_fmt != AV_PIX_FMT_YUV420P)
            throw std::runtime_error("WAS EXPECTING A YUV420P PIXEL FORMAT");

        int bufsize = av_image_alloc(
            video_dst_data_,
            video_dst_linesize_,
            video_codec_context_->width,
            video_codec_context_->height,
            video_codec_context_->pix_fmt,
            1);

        if (bufsize < 0)
        {
            char errbuf[AV_ERROR_MAX_STRING_SIZE];
            av_strerror(bufsize, errbuf, AV_ERROR_MAX_STRING_SIZE);
            throw std::runtime_error(std::string("CANNOT ALLOCATE IMAGE: ") + errbuf);
        }

        video_image_ = std::make_unique<image>(video_codec_context_->width, video_codec_context_->height);

        double aspect = video_codec_context_->width / (double)video_codec_context_->height;
        if (aspect > 512 / 342.0)
        {
            default_image_ = std::make_unique<image>(342 * aspect, 342);
        }
        else
        {
            default_image_ = std::make_unique<image>(512, 512 / aspect);
        }

        if (sDebug)
        {
            std::clog << "Image structure:\n";
            std::clog << video_dst_linesize_[0] << " " << video_dst_linesize_[1] << " " << video_dst_linesize_[2] << " " << video_dst_linesize_[3] << "\n";
            std::clog << (video_dst_linesize_[0] + video_dst_linesize_[1] + video_dst_linesize_[2] + video_dst_linesize_[3]) * video_codec_context_->height << "\n";
            std::clog << bufsize << "\n";
        }

        frame_ = av_frame_alloc();
        if (!frame_)
            throw std::runtime_error("Could not allocate frame");

        pkt_ = av_packet_alloc();
        if (!pkt_)
            throw std::runtime_error("Could not allocate packet");

        int got_frame = 0;

        while (av_read_frame(format_context_, pkt_) >= 0)
        {
            do
            {
                auto ret = decode_packet(&got_frame, pkt_);
                if (images_.size() >= frame_to_extract_)
                    goto end;
                if (ret < 0)
                    break;
            } while (pkt_->size > 0);
            av_packet_unref(pkt_);
        }

        // Flush the decoders
        pkt_->data = NULL;
        pkt_->size = 0;
        do {
            decode_packet(&got_frame, pkt_);
        } while (got_frame);

    end:
        std::clog << "\n";

        image_ix = 0;

        if (sDebug)
            std::clog << "\nAcquired " << images_.size() << " frames of video for a total of " << images_.size() / av_q2d(video_stream_->r_frame_rate) << " seconds \n";

        if (ixa != AVERROR_STREAM_NOT_FOUND && sound_)
        {
            sound_->process();
            sound_ix = 0;
        }

        if (sDebug)
            std::clog << "\n";
    }

    ~ffmpeg_reader()
    {
        avformat_close_input(&format_context_);
        if (sDebug)
            std::clog << "Closed media file\n";
        if (video_codec_context_)
            avcodec_free_context(&video_codec_context_);
        if (audio_codec_context_)
            avcodec_free_context(&audio_codec_context_);
        if (frame_)
            av_frame_free(&frame_);
        if (pkt_)
            av_packet_free(&pkt_);
        if (video_dst_data_[0])
            av_freep(&video_dst_data_[0]);
    }

    virtual double frame_rate() override
    {
        return av_q2d(video_stream_->r_frame_rate);
    }

    virtual std::unique_ptr<image> next() override
    {
        if (sDebug) std::clog << "next() called, image_ix: " << image_ix << ", images_.size(): " << images_.size() << std::endl;
        
        if (image_ix == -1 || image_ix >= (int)images_.size())
        {
            if (sDebug) std::clog << "next() returning nullptr" << std::endl;
            return nullptr;
        }
        auto res = std::make_unique<image>(images_[image_ix]);
        image_ix++;
        if (sDebug) std::clog << "next() returning image, new image_ix: " << image_ix << std::endl;
        return res;
    }

    virtual std::unique_ptr<sound_frame_t> next_sound() override
    {
        if (ixa != AVERROR_STREAM_NOT_FOUND && sound_)
        {
            return sound_->extract(sound_ix++);
        }
        return nullptr;
    }
};

std::unique_ptr<input_reader> make_ffmpeg_reader(const std::string &movie_path, double from, double to)
{
    try
    {
        return std::make_unique<ffmpeg_reader>(movie_path, from, to - from);
    }
    catch (const std::exception& e)
    {
        std::cerr << "**** ERROR: " << e.what() << std::endl;
        return nullptr;
    }
}