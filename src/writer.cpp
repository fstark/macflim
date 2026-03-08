#include "writer.hpp"

#include "errors.hpp"

#include <format>
#include <iostream>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
#include <libavutil/time.h>
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
            avformat_free_context(ctx);
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

using AVFormatContextPtr = std::unique_ptr<AVFormatContext, AVFormatContextDeleter>;
using AVCodecContextPtr = std::unique_ptr<AVCodecContext, AVCodecContextDeleter>;
using AVFramePtr = std::unique_ptr<AVFrame, AVFrameDeleter>;
} // namespace

class ffmpeg_writer final : public output_writer
{
    size_t W_; // Should be in output_writer
    size_t H_;

    /* check that a given sample format is supported by the encoder */
    static int check_sample_fmt(const AVCodec *codec, enum AVSampleFormat sample_fmt)
    {
        const enum AVSampleFormat *p = codec->sample_fmts;
        while (*p != AV_SAMPLE_FMT_NONE)
        {
            if (sDebug)
                std::clog << std::format("[{}] ", av_get_sample_fmt_name(*p));
            if (*p == sample_fmt)
                return 1;
            p++;
        }
        return 0;
    }

    AVFramePtr videoFrame;
    AVFramePtr audio_frame;
    AVCodecContextPtr video_context;
    AVCodecContextPtr audio_context;
    int frameCounter = 0;
    AVFormatContextPtr ofctx;
    const AVOutputFormat *oformat = nullptr;

    // Small state for audio encoding (22100 in 370 u8 sample to 44200 in 1024 flt samples)
    float audio_44[735];
    size_t audio_pos = 0;
    int audio_frame_counter = 0;

    void pushFrame(const grayscale &img, const sound_frame_t &snd)
    {
        int err;
        if (!videoFrame)
        {
            videoFrame.reset(av_frame_alloc());
            if (!videoFrame)
            {
                throw flim_error("Failed to allocate video frame");
            }
            videoFrame->format = AV_PIX_FMT_YUV420P;
            videoFrame->width = video_context->width;
            videoFrame->height = video_context->height;
            if ((err = av_frame_get_buffer(videoFrame.get(), 32)) < 0)
            {
                std::cerr << std::format("Failed to allocate picture buffer: {}\n", err);
                return;
            }
            av_frame_make_writable(videoFrame.get());

            memset(videoFrame->data[1], 128, H_ / 2 * videoFrame->linesize[1]);
            memset(videoFrame->data[2], 128, H_ / 2 * videoFrame->linesize[2]);
        }

        uint8_t *p = videoFrame->data[0];
        for (size_t y = 0; y != H_; y++)
        {
            for (size_t x = 0; x != W_; x++)
            {
                auto v = img.at(x, y);
                *p++ = v <= 0.5 ? 0 : 255;
            }
        }

        videoFrame->pts = 1500 * frameCounter;

        if ((err = avcodec_send_frame(video_context.get(), videoFrame.get())) < 0)
        {
            std::cerr << std::format("Failed to send frame: {}\n", err);
            return;
        }

        AVPacket pkt;
        av_init_packet(&pkt);
        pkt.data = NULL;
        pkt.size = 0;
        pkt.flags |= AV_PKT_FLAG_KEY;
        if (avcodec_receive_packet(video_context.get(), &pkt) == 0)
        {
            av_interleaved_write_frame(ofctx.get(), &pkt);
            av_packet_unref(&pkt);
        }

        float *audio_p = (float *)audio_frame->data[0];

        // AUDIO
        // We convert to 44KHz
        for (int i = 0; i != 735; i++)
        {
            audio_44[i] = (*(snd.begin() + (int)(i / 735.0 * 370)) - 128.0) / 128;
        }

        // How many samples left to send?
        if (audio_pos + 735 >= 1024)
        {
            memcpy(audio_p + audio_pos, audio_44, (1024 - audio_pos) * sizeof(float));

            audio_frame->pts = audio_frame_counter * 1024;
            audio_frame_counter++;

            err = avcodec_send_frame(audio_context.get(), audio_frame.get());
            if (err < 0)
                throw ffmpeg_error("Error sending the frame to the encoder", err);

            err = avcodec_receive_packet(audio_context.get(), &pkt);
            if (err == AVERROR(EAGAIN) || err == AVERROR_EOF)
                return;
            else if (err < 0)
                throw ffmpeg_error("Error encoding audio frame", err);

            pkt.stream_index = 1; // Corrected this line

            av_interleaved_write_frame(ofctx.get(), &pkt);
            av_packet_unref(&pkt);

            memcpy(audio_p, audio_44 + 1024 - audio_pos, (735 - 1024 + audio_pos) * sizeof(float));
            audio_pos = 735 - 1024 + audio_pos;
        }
        else
        {
            memcpy(audio_p + audio_pos, audio_44, 735 * sizeof(float));
            audio_pos += 735;
        }

        frameCounter++;
    }

    static void dump_codecs()
    {
        void *i = 0;
        const AVCodec *p;

        while ((p = av_codec_iterate(&i)))
        {
            std::clog << std::format("{} ", p->name);
        }

        std::clog << std::format("\n");
    }

    void init_output_format(const std::string &filename)
    {
        oformat = av_guess_format(nullptr, filename.c_str(), nullptr);
        if (!oformat)
            throw io_error("Can't create output format", filename);

        AVFormatContext *raw_ofctx = nullptr;
        int err = avformat_alloc_output_context2(&raw_ofctx, oformat, nullptr, filename.c_str());
        if (err < 0)
            throw ffmpeg_error("Can't create output context", err, filename);
        ofctx.reset(raw_ofctx);
    }

    void init_video_stream()
    {
        const AVCodec *video_codec = avcodec_find_encoder(oformat->video_codec);
        if (!video_codec)
            throw flim_error("Can't create video codec");

        AVStream *stream = avformat_new_stream(ofctx.get(), video_codec);
        if (!stream)
            throw flim_error("Can't create video stream");

        video_context.reset(avcodec_alloc_context3(video_codec));
        if (!video_context)
            throw flim_error("Can't create video codec context");

        stream->codecpar->codec_id = oformat->video_codec;
        stream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
        stream->codecpar->width = W_;
        stream->codecpar->height = H_;
        stream->codecpar->format = AV_PIX_FMT_YUV420P;
        stream->codecpar->bit_rate = 60 * 6000 * 8;

        int ret = avcodec_parameters_to_context(video_context.get(), stream->codecpar);
        if (ret < 0)
            throw ffmpeg_error("Failed to copy codec parameters to context", ret);

        video_context->time_base = (AVRational){1, 30};
        video_context->max_b_frames = 2;
        video_context->gop_size = 12;
        video_context->framerate = (AVRational){60, 1};

        if (stream->codecpar->codec_id == AV_CODEC_ID_H264)
            av_opt_set(video_context.get(), "preset", "ultrafast", 0);
        else if (stream->codecpar->codec_id == AV_CODEC_ID_H265)
            av_opt_set(video_context.get(), "preset", "ultrafast", 0);

        avcodec_parameters_from_context(stream->codecpar, video_context.get());

        int err = avcodec_open2(video_context.get(), video_codec, NULL);
        if (err < 0)
            throw ffmpeg_error("Failed to open codec", err);
    }

    void init_audio_stream()
    {
        auto audio_codec = avcodec_find_encoder(oformat->audio_codec);
        if (!audio_codec)
            throw flim_error("Audio codec not found");

        audio_context.reset(avcodec_alloc_context3(audio_codec));
        if (!audio_context)
            throw flim_error("Could not allocate audio codec context");

        audio_context->sample_fmt = AV_SAMPLE_FMT_FLTP;
        if (!check_sample_fmt(audio_codec, audio_context->sample_fmt))
            throw flim_error("Encoder does not support FLT planar samples");

        audio_context->sample_rate = 44100;
        audio_context->ch_layout = (AVChannelLayout)AV_CHANNEL_LAYOUT_MONO;
        audio_context->ch_layout.nb_channels = 1;

        int ret = avcodec_open2(audio_context.get(), audio_codec, NULL);
        if (ret < 0)
            throw ffmpeg_error("Could not open audio codec", ret);

        AVStream *audio_stream = avformat_new_stream(ofctx.get(), audio_codec);
        if (!audio_stream)
            throw flim_error("Cannot create audio stream");

        audio_stream->id = 1;
        audio_stream->time_base = (AVRational){1, 44100};
        avcodec_parameters_from_context(audio_stream->codecpar, audio_context.get());

        audio_frame.reset(av_frame_alloc());
        if (!audio_frame)
            throw flim_error("Error allocating an audio frame");

        audio_frame->format = audio_context->sample_fmt;
        audio_frame->ch_layout = audio_context->ch_layout;
        audio_frame->sample_rate = audio_context->sample_rate;
        audio_frame->nb_samples = 1024;
        int err = av_frame_get_buffer(audio_frame.get(), 0);
        if (err < 0)
            throw ffmpeg_error("Error allocating an audio buffer", err);

        if (sDebug)
        {
            std::clog << std::format("Line size = {}\n", audio_frame->linesize[0]);
            std::clog << std::format("Frame size = {}\n", audio_context->frame_size);
        }
    }

    void open_output_file(const std::string &filename)
    {
        if (!(oformat->flags & AVFMT_NOFILE))
        {
            int err = avio_open(&ofctx->pb, filename.c_str(), AVIO_FLAG_WRITE);
            if (err < 0)
                throw ffmpeg_error("Failed to open file", err, filename);
        }

        int err = avformat_write_header(ofctx.get(), NULL);
        if (err < 0)
        {
            char buffer[1025];
            av_strerror(err, buffer, 1024);
            std::cerr << std::format("{}: {}\n", err, buffer);
            throw ffmpeg_error("Failed to write header", err, filename);
        }

        av_dump_format(ofctx.get(), 0, filename.c_str(), 1);
    }

  public:
    ffmpeg_writer(const std::string filename, size_t W, size_t H) : W_(W), H_(H)
    {
        init_output_format(filename);
        init_video_stream();
        init_audio_stream();
        open_output_file(filename);
    }

    ~ffmpeg_writer()
    {
        if (sDebug)
            std::clog << std::format("~ffmpeg_writer()\n");

        // DELAYED FRAMES
        AVPacket pkt;
        av_init_packet(&pkt);
        pkt.data = NULL;
        pkt.size = 0;

        for (;;)
        {
            avcodec_send_frame(video_context.get(), NULL);
            if (avcodec_receive_packet(video_context.get(), &pkt) == 0)
            {
                av_interleaved_write_frame(ofctx.get(), &pkt);
                av_packet_unref(&pkt);
            }
            else
            {
                break;
            }
        }

        av_write_trailer(ofctx.get());
        if (!(oformat->flags & AVFMT_NOFILE))
        {
            int err = avio_close(ofctx->pb);
            if (err < 0)
            {
                std::cerr << std::format("Failed to close file: {}\n", err);
            }
        }

        // Smart pointers automatically clean up videoFrame, audio_frame, video_context, audio_context, ofctx

        if (sDebug)
            std::clog << std::format("#### End of video stream\n");
    }

    void write_frame(const grayscale &img, const sound_frame_t &snd) override
    {
        pushFrame(img, snd);
    }
};

class gif_writer final : public output_writer
{
    size_t count_ = 0;
    size_t num_ = 0;
    std::string filename_;

  public:
    gif_writer(const std::string filename) : filename_{filename} {}

    void write_frame(const grayscale &img, [[maybe_unused]] const sound_frame_t &snd) override
    {
        if ((count_ % 3) == 0)
        {
            std::string buffer = std::format("/tmp/gif-{:06}.pgm", num_);
            write_grayscale(buffer.c_str(), img);
            num_++;
        }
        count_++;
    }

    ~gif_writer()
    {
        std::string buffer = std::format("convert -delay 5 -loop 0 /tmp/gif-*.pgm '{}'", filename_);
        std::clog << std::format("GENERATING GIF FILE\n");
        int res = system(buffer.c_str());
        if (res != 0)
        {
            std::cerr << std::format("**** FAILED TO GENERATE GIF FILE (retcode={})\n", res);
        }
        std::clog << std::format("DONE\n");
        delete_files_of_pattern("/tmp/gif-%06d.pgm");
    }
};

std::unique_ptr<output_writer> make_ffmpeg_writer(const std::string &movie_path, size_t w, size_t h)
{
    return std::make_unique<ffmpeg_writer>(movie_path, w, h);
}

std::unique_ptr<output_writer> make_gif_writer(const std::string &movie_path, [[maybe_unused]] size_t w,
                                               [[maybe_unused]] size_t h)
{
    return std::make_unique<gif_writer>(movie_path);
}

class pgm_writer final : public output_writer
{
    std::string pattern_;
    size_t frame_num_ = 1;

  public:
    pgm_writer(const std::string &pattern) : pattern_{pattern}
    {
        // Clean up old files matching this pattern
        delete_files_of_pattern(pattern_);
    }

    void write_frame(const grayscale &img, [[maybe_unused]] const sound_frame_t &snd) override
    {
        std::string buffer = std::vformat(pattern_, std::make_format_args(frame_num_));
        write_grayscale(buffer.c_str(), img);
        frame_num_++;
    }
};

std::unique_ptr<output_writer> make_pgm_writer(const std::string &pattern)
{
    return std::make_unique<pgm_writer>(pattern);
}

class null_writer final : public output_writer
{
  public:
    void write_frame([[maybe_unused]] const grayscale &img, [[maybe_unused]] const sound_frame_t &snd) override {}
};

std::unique_ptr<output_writer> make_null_writer()
{
    return std::make_unique<null_writer>();
}

} // namespace macflim