#include "writer.hpp"

#include <iostream>

extern "C"{
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libavutil/avutil.h>
    #include <libavutil/time.h>
    #include <libavutil/opt.h>
}

class ffmpeg_writer : public output_writer
{

AVFrame* videoFrame = nullptr;
AVCodecContext* cctx = nullptr;
int frameCounter = 0;
AVFormatContext* ofctx = nullptr;
AVOutputFormat* oformat = nullptr;

void pushFrame( const image &img )
{
    int err;
    if (!videoFrame)
    {
        videoFrame = av_frame_alloc();
        videoFrame->format = AV_PIX_FMT_YUV420P;
        videoFrame->width = cctx->width;
        videoFrame->height = cctx->height;
        if ((err = av_frame_get_buffer(videoFrame, 32)) < 0)
        {
            std::cout << "Failed to allocate picture" << err << std::endl;
            return;
        }
        av_frame_make_writable(videoFrame);

        std::cout << videoFrame->linesize[0] << " ";
        std::cout << videoFrame->linesize[1] << " ";
        std::cout << videoFrame->linesize[2] << "\n";

        // for (int i=0;i!=342;i++)
        //     memset( videoFrame->data[0]+512*i, i, 512 );

        memset( videoFrame->data[1], 128, 342/2*videoFrame->linesize[1] );
        memset( videoFrame->data[2], 128, 342/2*videoFrame->linesize[2] );
    }

        uint8_t *p = videoFrame->data[0];
        for (int y=0;y!=342;y++)
            for (int x=0;x!=512;x++)
            {
                auto v = img.at(x,y);
                if (v<=0.5) *p++ = 0;
                else *p++ = 255;
            }

    videoFrame->pts = 1500 * (frameCounter++);  //  #### WHY??? (voice of Bronsky Beat)

    // std::clog << "TB=" << av_q2d(cctx->time_base) << "\n";

// exit(0);

    // std::cout << videoFrame->pts << " " << cctx->time_base.num << " " << 
    //     cctx->time_base.den << " " << frameCounter << std::endl;
    if ((err = avcodec_send_frame(cctx, videoFrame)) < 0) {
        std::cout << "Failed to send frame" << err << std::endl;
        return;
    }
    // AV_TIME_BASE;
    AVPacket pkt;
    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;
    pkt.flags |= AV_PKT_FLAG_KEY;
    if (avcodec_receive_packet(cctx, &pkt) == 0) {
        // static int counter = 0;
        // if (counter == 0) {
        //     FILE* fp = fopen("dump_first_frame1.dat", "wb");
        //     fwrite(pkt.data, pkt.size, 1, fp);
        //     fclose(fp);
        //     counter++;
        // }
        // std::cout << "pkt key: " << (pkt.flags & AV_PKT_FLAG_KEY) << " " << 
        //     pkt.size << "\n";
        uint8_t* size = ((uint8_t*)pkt.data);
        // std::cout << "first: " << (int)size[0] << " " << (int)size[1] << 
        //     " " << (int)size[2] << " " << (int)size[3] << " " << (int)size[4] << 
        //     " " << (int)size[5] << " " << (int)size[6] << " " << (int)size[7] << 
        //     std::endl;
        av_interleaved_write_frame(ofctx, &pkt);
        av_packet_unref(&pkt);
    }
}

static AVCodec *dump_codecs()
{
    void *i = 0;
    const AVCodec *p;
  
    while ((p = av_codec_iterate(&i)))
    {
        if (p->encode2 /* || p->send_frame */)
            std::clog << p->name << " ";
    }

    std::clog << "\n";

    return NULL;
 }

public:
    ffmpeg_writer( const std::string filename )
    {
    av_register_all();

    oformat = av_guess_format(nullptr, filename.c_str(), nullptr);
    if (!oformat)
    {
        throw "Can't create output format";
    }

    int err = avformat_alloc_output_context2(&ofctx, oformat, nullptr, filename.c_str());
    if (err)
    {
        throw "can't create output context";
    }

    AVCodec* codec = nullptr;
    codec = avcodec_find_encoder(oformat->video_codec);
    if (!codec)
    {
        throw "Can't create codec";
    }

    AVStream* stream = avformat_new_stream(ofctx, codec);
    if (!stream)
    {
        std::cout << "can't find format" << std::endl;
    }

    cctx = avcodec_alloc_context3(codec);

    if (!cctx)
    {
        throw "Can't create codec context";
    }

    stream->codecpar->codec_id = oformat->video_codec;
    stream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
    stream->codecpar->width = 512;
    stream->codecpar->height = 342;
    stream->codecpar->format = AV_PIX_FMT_YUV420P;
    stream->codecpar->bit_rate = 60 * 6000 * 8;
    avcodec_parameters_to_context(cctx, stream->codecpar);
    cctx->time_base = (AVRational){ 1, 30 };
    cctx->max_b_frames = 2;
    cctx->gop_size = 12;
    cctx->framerate = (AVRational){ 60, 1 };

    //must remove the following
    // cctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    if (stream->codecpar->codec_id == AV_CODEC_ID_H264)
    {
        av_opt_set(cctx, "preset", "ultrafast", 0);
    }
    else if (stream->codecpar->codec_id == AV_CODEC_ID_H265)
    {
        av_opt_set(cctx, "preset", "ultrafast", 0);
    }

    avcodec_parameters_from_context(stream->codecpar, cctx);

    if ((err = avcodec_open2(cctx, codec, NULL)) < 0)
    {
        throw "Failed to open codec";
    }

    if (!(oformat->flags & AVFMT_NOFILE))
    {
        if ((err = avio_open(&ofctx->pb, filename.c_str(), AVIO_FLAG_WRITE)) < 0)
        {
            throw "Failed to open file";
        }
    }

    if ((err = avformat_write_header(ofctx, NULL)) < 0)
    {
        throw "Failed to write header";
    }

    av_dump_format(ofctx, 0, filename.c_str(), 1);

    }

    ~ffmpeg_writer()
    {

    std::clog << "~ffmpeg_writer()\n";

    //DELAYED FRAMES
    AVPacket pkt;
    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;

    for (;;) {
        avcodec_send_frame(cctx, NULL);
        if (avcodec_receive_packet(cctx, &pkt) == 0) {
            av_interleaved_write_frame(ofctx, &pkt);
            av_packet_unref(&pkt);
        }
        else {
            break;
        }
    }

    av_write_trailer(ofctx);
    if (!(oformat->flags & AVFMT_NOFILE))
    {
        int err = avio_close(ofctx->pb);
        if (err < 0) {
            std::cout << "Failed to close file" << err << std::endl;
        }
    }

    if (videoFrame)
    {
        av_frame_free(&videoFrame);
    }
    if (cctx)
    {
        avcodec_free_context(&cctx);
    }
    if (ofctx)
    {
        avformat_free_context(ofctx);
    }

    std::clog << "#### End of video stream\n";
}
    virtual void write_frame( const image& img, const sound_frame_t &snd )
    {
        pushFrame( img );
    }
};

std::unique_ptr<output_writer> make_ffmpeg_writer( const std::string &movie_path, size_t w, size_t h )
{
    return std::make_unique<ffmpeg_writer>( movie_path );
}

class null_writer : public output_writer
{
public:
    virtual void write_frame( const image& img, const sound_frame_t &snd )
    {
    }
};

std::unique_ptr<output_writer> make_null_writer()
{
    return std::make_unique<null_writer>();
}
