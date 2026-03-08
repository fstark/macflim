#include "ffmpeg_reader.hpp"

#include "errors.hpp"

#include <array>
#include <format>
#include <optional>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
}

namespace macflim
{
extern bool sDebug;

// RAII wrappers for FFmpeg resources
namespace
{
struct AVFormatContextDeleter
{
    void operator()(AVFormatContext *ctx) const
    {
        if (ctx)
            avformat_close_input(&ctx);
    }
};

struct AVCodecContextDeleter
{
    void operator()(AVCodecContext *ctx) const
    {
        if (ctx)
            avcodec_free_context(&ctx);
    }
};

struct AVFrameDeleter
{
    void operator()(AVFrame *frame) const
    {
        if (frame)
            av_frame_free(&frame);
    }
};

struct AVPacketDeleter
{
    void operator()(AVPacket *pkt) const
    {
        if (pkt)
            av_packet_free(&pkt);
    }
};

using AVFormatContextPtr = std::unique_ptr<AVFormatContext, AVFormatContextDeleter>;
using AVCodecContextPtr = std::unique_ptr<AVCodecContext, AVCodecContextDeleter>;
using AVFramePtr = std::unique_ptr<AVFrame, AVFrameDeleter>;
using AVPacketPtr = std::unique_ptr<AVPacket, AVPacketDeleter>;
} // namespace

/// This stores a sound buffer and transform it into a suitable format for flims
class sound_buffer
{
    std::vector<float> data_;
    size_t channel_count_ = 0; //  # of channels
    size_t sample_rate_ = 0;   //  # of samples per second
    float min_sample_;
    float max_sample_;

  public:
    sound_buffer(size_t channel_count, size_t sample_rate) : channel_count_{channel_count}, sample_rate_{sample_rate} {}

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
class ffmpeg_reader final : public input_reader
{
    AVFormatContextPtr format_context_;
    const AVCodec *video_decoder_;
    AVStream *video_stream_;
    AVCodecContextPtr video_codec_context_;
    uint8_t *video_dst_data_[4] = {NULL};
    int video_dst_linesize_[4];
    AVPacketPtr pkt_;
    AVFramePtr frame_;
    int ixv; //  Video stream index
    size_t video_frame_count_ = 0;
    std::unique_ptr<grayscale> video_image_;   //  Size of the video input
    std::unique_ptr<grayscale> default_image_; //  Size of our output
    double first_frame_second_;
    size_t frame_to_extract_;
    bool done_ = false; //  No more frames to decode

    /// Try to receive one decoded video frame.  Returns true if a frame was produced.
    bool receive_video_frame(grayscale &out)
    {
        for (;;)
        {
            int ret = avcodec_receive_frame(video_codec_context_.get(), frame_.get());
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

            av_image_copy(video_dst_data_, video_dst_linesize_, (const uint8_t **)(frame_->data), frame_->linesize,
                          video_codec_context_->pix_fmt, video_codec_context_->width, video_codec_context_->height);

            video_image_->set_luma(video_dst_data_[0]);

            out = *default_image_;
            copy(out, *video_image_, true, 0.5, 0.5);
            video_frame_count_++;
            //            std::clog << "Read " << video_frame_count_ << " frames\r" << std::flush;
            return true;
        }
    }

    /// Send a video packet to the decoder with error handling.
    /// Returns true if packet was successfully sent (or EAGAIN/EOF which are recoverable).
    /// Sets done_ flag and returns false on fatal errors.
    bool send_video_packet_checked(AVPacket *pkt)
    {
        int ret = avcodec_send_packet(video_codec_context_.get(), pkt);
        av_packet_unref(pkt);

        if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF)
        {
            char errbuf[AV_ERROR_MAX_STRING_SIZE];
            av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
            std::cerr << "Error sending video packet: " << errbuf << std::endl;
            done_ = true;
            return false;
        }

        return true;
    }

    void init_video_context()
    {
        video_codec_context_.reset(avcodec_alloc_context3(video_decoder_));
        if (!video_codec_context_)
            throw flim_error("Cannot allocate video codec context");

        int ret = avcodec_parameters_to_context(video_codec_context_.get(), video_stream_->codecpar);
        if (ret < 0)
            throw ffmpeg_error("Failed to copy video codec parameters", ret);

        AVDictionary *opts = NULL;
        av_dict_set(&opts, "refcounted_frames", "0", 0); //  Do not refcount

        int ret2 = avcodec_open2(video_codec_context_.get(), video_decoder_, &opts);
        if (ret2 < 0)
            throw ffmpeg_error("Cannot open video codec", ret2);

        if (sDebug)
        {
            std::clog << std::format("VIDEO CODEC OPENED WITH PIXEL FORMAT {}\n",
                                     av_get_pix_fmt_name(video_codec_context_->pix_fmt));
        }

        if (video_codec_context_->pix_fmt != AV_PIX_FMT_YUV420P)
            throw flim_error("Expected YUV420P pixel format");
    }

    void open_media_file(const std::string &movie_path)
    {
        AVFormatContext *raw_ctx = nullptr;
        int ret = avformat_open_input(&raw_ctx, movie_path.c_str(), NULL, NULL);
        if (ret != 0)
            throw ffmpeg_error("Cannot open input file", ret, movie_path);
        format_context_.reset(raw_ctx);

        int ret2 = avformat_find_stream_info(format_context_.get(), NULL);
        if (ret2 < 0)
            throw ffmpeg_error("Cannot find stream information", ret2, movie_path);

        if (sDebug)
            std::clog << std::format("Searching for video in {} streams\n", format_context_->nb_streams);

        ixv = av_find_best_stream(format_context_.get(), AVMEDIA_TYPE_VIDEO, -1, -1, &video_decoder_, 0);

        if (ixv == AVERROR_STREAM_NOT_FOUND)
            throw ffmpeg_error("No video stream found in file", ixv, movie_path);
        if (ixv == AVERROR_DECODER_NOT_FOUND)
            throw ffmpeg_error("No suitable video decoder available", ixv, movie_path);

        if (sDebug)
            std::clog << std::format("Video stream index: {}\n", ixv);
    }

    void seek_to_start(const std::string &movie_path, timestamp_t from, timestamp_t duration)
    {
        timestamp_t actual_duration = format_context_->duration / (double)AV_TIME_BASE;
        if (duration > actual_duration)
        {
            std::clog << std::format(
                "Warning: Requested duration ({}s) exceeds video length ({}s). Trimming to video length.\n", duration,
                actual_duration);
            duration = actual_duration;
        }

        timestamp_t seek_to = std::max(from - 10.0, 0.0);
        int ret = avformat_seek_file(format_context_.get(), -1, seek_to * AV_TIME_BASE, seek_to * AV_TIME_BASE,
                                     seek_to * AV_TIME_BASE, AVSEEK_FLAG_ANY);
        if (ret < 0)
            throw ffmpeg_error("Cannot seek in file", ret, movie_path);

        video_stream_ = format_context_->streams[ixv];

        if (sDebug)
        {
            std::clog << std::format("Video: {}x{}@{} fps timebase:{}\n", video_stream_->codecpar->width,
                                     video_stream_->codecpar->height, av_q2d(video_stream_->r_frame_rate),
                                     av_q2d(video_stream_->time_base));
        }

        first_frame_second_ = from;
        frame_to_extract_ = duration * av_q2d(video_stream_->r_frame_rate);
    }

    void allocate_buffers()
    {
        auto bufsize = av_image_alloc(video_dst_data_, video_dst_linesize_, video_codec_context_->width,
                                      video_codec_context_->height, video_codec_context_->pix_fmt, 1);
        if (bufsize < 0)
            throw ffmpeg_error("Cannot allocate image buffer", bufsize);

        video_image_ = std::make_unique<grayscale>(video_codec_context_->width, video_codec_context_->height);

        // Compute output dimensions preserving aspect ratio relative to 512x342
        double aspect = video_codec_context_->width / (double)video_codec_context_->height;
        if (aspect > 512 / 342.0)
            default_image_ = std::make_unique<grayscale>(342 * aspect, 342);
        else
            default_image_ = std::make_unique<grayscale>(512, 512 / aspect);

        if (sDebug)
        {
            std::clog << std::format(
                "Image structure:\n{} {} {} {}\n{}\n{}\n", video_dst_linesize_[0], video_dst_linesize_[1],
                video_dst_linesize_[2], video_dst_linesize_[3],
                (video_dst_linesize_[0] + video_dst_linesize_[1] + video_dst_linesize_[2] + video_dst_linesize_[3]) *
                    video_codec_context_->height,
                bufsize);
        }

        frame_.reset(av_frame_alloc());

        pkt_.reset(av_packet_alloc());
        if (!pkt_)
            throw flim_error("Failed to allocate packet");
    }

  public:
    ffmpeg_reader(const std::string &movie_path, timestamp_t from, timestamp_t duration)
    {
        av_log_set_level(AV_LOG_WARNING);
        open_media_file(movie_path);
        seek_to_start(movie_path, from, duration);
        init_video_context();
        allocate_buffers();
    }

    ~ffmpeg_reader()
    {
        // Smart pointers automatically clean up format_context_, video_codec_context_, frame_, pkt_
        av_freep(&video_dst_data_[0]);
        if (sDebug)
        {
            std::clog << std::format("Closed media file\n");
        }
    }

    double frame_rate() override
    {
        return av_q2d(video_stream_->r_frame_rate);
    }

    /// Lazily decode and return the next video frame, or nullptr when done.
    std::unique_ptr<grayscale> next() override
    {
        if (done_)
            return nullptr;

        if (video_frame_count_ >= frame_to_extract_)
        {
            done_ = true;
            return nullptr;
        }

        auto result = std::make_unique<grayscale>(default_image_->W(), default_image_->H());

        // Try to get a frame from already-sent packets
        if (receive_video_frame(*result))
            return result;

        // Read more packets until we get a video frame
        while (av_read_frame(format_context_.get(), pkt_.get()) >= 0)
        {
            if (pkt_->stream_index == ixv)
            {
                if (!send_video_packet_checked(pkt_.get()))
                    return nullptr;

                if (receive_video_frame(*result))
                {
                    if (video_frame_count_ >= frame_to_extract_)
                        done_ = true;
                    return result;
                }
            }
            else
            {
                av_packet_unref(pkt_.get()); // Skip non-video packets
            }
        }

        // No more packets — flush the decoder
        avcodec_send_packet(video_codec_context_.get(), nullptr);
        if (receive_video_frame(*result))
            return result;

        done_ = true;
        return nullptr;
    }

    /// Audio is handled by the separate decode_audio() function
    std::unique_ptr<sound_frame_t> next_sound() override
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
    catch (const std::exception &e)
    {
        std::cerr << std::format("**** ERROR: {}\n", e.what());
        return nullptr;
    }
}

/// Bundles the FFmpeg state needed to decode an audio stream.
struct audio_decode_context
{
    AVFormatContextPtr format_context;
    AVCodecContextPtr codec_context;
    AVStream *stream = nullptr;
    int stream_index = -1;
};

/// Open a media file and prepare its audio stream for decoding.
/// Returns nullopt if the file has no audio stream.
std::optional<audio_decode_context> open_audio_stream(const std::string &movie_path)
{
    av_log_set_level(AV_LOG_WARNING);

    AVFormatContext *raw_ctx = nullptr;
    int ret = avformat_open_input(&raw_ctx, movie_path.c_str(), NULL, NULL);
    if (ret != 0)
        throw ffmpeg_error("Cannot open input file for audio", ret, movie_path);
    AVFormatContextPtr format_context(raw_ctx);

    int ret2 = avformat_find_stream_info(format_context.get(), NULL);
    if (ret2 < 0)
        throw ffmpeg_error("Cannot find stream information for audio", ret2, movie_path);

    const AVCodec *audio_decoder = nullptr;
    int ixa = av_find_best_stream(format_context.get(), AVMEDIA_TYPE_AUDIO, -1, -1, &audio_decoder, 0);

    if (ixa == AVERROR_STREAM_NOT_FOUND)
    {
        std::cerr << std::format("NO SOUND -- INSERTING SILENCE\n");
        return std::nullopt;
    }
    if (ixa == AVERROR_DECODER_NOT_FOUND)
        throw ffmpeg_error("No suitable audio decoder available", ixa, movie_path);

    AVCodecContextPtr codec_context(avcodec_alloc_context3(audio_decoder));
    if (!codec_context)
        throw flim_error("Cannot allocate audio codec context");

    int ret3 = avcodec_parameters_to_context(codec_context.get(), format_context->streams[ixa]->codecpar);
    if (ret3 < 0)
        throw ffmpeg_error("Failed to copy audio codec parameters", ret3);

    int ret4 = avcodec_open2(codec_context.get(), audio_decoder, nullptr);
    if (ret4 < 0)
        throw ffmpeg_error("Cannot open audio codec", ret4);

    if (sDebug)
    {
        std::clog << std::format("AUDIO CODEC: {}\n", avcodec_get_name(codec_context->codec_id));
        std::clog << std::format("SAMPLE FORMAT: {}\n", av_get_sample_fmt_name(codec_context->sample_fmt));
        std::clog << std::format("# CHANNELS: {}\n", codec_context->ch_layout.nb_channels);
        std::clog << std::format("SAMPLE RATE: {}\n", codec_context->sample_rate);
    }

    audio_decode_context ctx;
    ctx.stream = format_context->streams[ixa];
    ctx.stream_index = ixa;
    ctx.format_context = std::move(format_context);
    ctx.codec_context = std::move(codec_context);
    return ctx;
}

/// Convert a sound_buffer into Mac sound frames (370 bytes each at 1/60th second).
std::vector<sound_frame_t> convert_to_mac_frames(sound_buffer &sound)
{
    sound.process();

    std::clog << std::format("\nConverting audio to Mac format...\n");
    std::vector<sound_frame_t> result;
    for (size_t i = 0;; i++)
    {
        auto sf = sound.extract(i);
        if (!sf)
            break;
        result.push_back(*sf);
    }
    std::clog << std::format("Audio: {} sound frames ({:.2f}s)\n", result.size(), result.size() / 60.0);
    return result;
}

/// Decode only the audio stream from a media file, normalize, and convert to Mac sound frames.
std::vector<sound_frame_t> decode_audio(const std::string &movie_path, timestamp_t from, timestamp_t duration)
{
    auto ctx_opt = open_audio_stream(movie_path);
    if (!ctx_opt)
        return {};

    auto &ctx = *ctx_opt;
    int n_channels = ctx.codec_context->ch_layout.nb_channels;
    sound_buffer sound(n_channels, ctx.codec_context->sample_rate);

    std::clog << std::format("Audio stream: {}Hz, {} channel(s)\n", ctx.codec_context->sample_rate, n_channels);

    // Seek to just before the requested start
    timestamp_t seek_to = std::max(from - 10.0, 0.0);
    std::clog << std::format("Seeking to {:.1f}s (target: {:.1f}s, duration: {:.1f}s, end: {:.1f}s)\n", seek_to, from,
                             duration, from + duration);
    int ret = avformat_seek_file(ctx.format_context.get(), -1, seek_to * AV_TIME_BASE, seek_to * AV_TIME_BASE,
                                 seek_to * AV_TIME_BASE, AVSEEK_FLAG_ANY);
    if (ret < 0)
        throw ffmpeg_error("Cannot seek in file for audio", ret, movie_path);

    AVFramePtr frame(av_frame_alloc());
    AVPacketPtr pkt(av_packet_alloc());
    if (!pkt)
        throw flim_error("Failed to allocate packet for audio");

    bool found_sound = false;
    double end_time = from + duration;
    double last_log_time = from;

    // Shared logic: process a single decoded audio frame
    auto process_frame = [&](AVFrame *f) -> bool
    {
        double pts = f->pts * av_q2d(ctx.stream->time_base);
        if (pts >= end_time)
            return false; // signal caller to stop

        if (pts < from)
            return true; // skip, keep going

        if (pts - last_log_time >= 5.0)
        {
            std::clog << std::format("  Decoding audio: {:.1f}s / {:.1f}s ({:.0f}%)\r", pts, end_time,
                                     (pts - from) / duration * 100.0);
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
        sound.append_samples((float **)f->extended_data, f->nb_samples);
        return true;
    };

    // Receive all available decoded frames from the codec
    auto drain_decoder = [&]() -> bool
    {
        while (true)
        {
            int r = avcodec_receive_frame(ctx.codec_context.get(), frame.get());
            if (r == AVERROR(EAGAIN) || r == AVERROR_EOF)
                return true;
            if (r < 0)
                return true;
            if (!process_frame(frame.get()))
                return false;
        }
    };

    // Decode all audio packets
    std::clog << std::format("Decoding audio...\n");
    bool keep_going = true;
    while (keep_going && av_read_frame(ctx.format_context.get(), pkt.get()) >= 0)
    {
        if (pkt->stream_index != ctx.stream_index)
        {
            av_packet_unref(pkt.get());
            continue;
        }
        int r = avcodec_send_packet(ctx.codec_context.get(), pkt.get());
        av_packet_unref(pkt.get());
        if (r < 0 && r != AVERROR(EAGAIN) && r != AVERROR_EOF)
            continue;
        keep_going = drain_decoder();
    }

    // Flush the decoder
    avcodec_send_packet(ctx.codec_context.get(), nullptr);
    drain_decoder();

    return convert_to_mac_frames(sound);
}

} // namespace macflim
