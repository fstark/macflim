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


/* check that a given sample format is supported by the encoder */
static int check_sample_fmt(const AVCodec *codec, enum AVSampleFormat sample_fmt)
{
    const enum AVSampleFormat *p = codec->sample_fmts;
    while (*p != AV_SAMPLE_FMT_NONE) {
        std::clog << "[ " << av_get_sample_fmt_name(*p) << "] ";
        if (*p == sample_fmt)
            return 1;
        p++;
    }
    return 0;
}

AVFrame *videoFrame = nullptr;
AVFrame *audio_frame = nullptr;
AVCodecContext* video_context = nullptr;
AVCodecContext* audio_context = nullptr;
int frameCounter = 0;
AVFormatContext* ofctx = nullptr;
AVOutputFormat* oformat = nullptr;

    //  Small state for audio encoding (22100 in 370 u8 sample to 44200 in 1024 flt samples)
float audio_44[735];
size_t audio_pos = 0;
int audio_frame_counter = 0;


void pushFrame( const image &img, const sound_frame_t &snd )
{
    int err;
    if (!videoFrame)
    {
        videoFrame = av_frame_alloc();
        videoFrame->format = AV_PIX_FMT_YUV420P;
        videoFrame->width = video_context->width;
        videoFrame->height = video_context->height;
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

    videoFrame->pts = 1500 * frameCounter;  //  #### WHY??? (voice of Bronsky Beat)

    // std::clog << "TB=" << av_q2d(cctx->time_base) << "\n";

// exit(0);

    // std::cout << videoFrame->pts << " " << cctx->time_base.num << " " << 
    //     cctx->time_base.den << " " << frameCounter << std::endl;
    if ((err = avcodec_send_frame(video_context, videoFrame)) < 0) {
        std::cout << "Failed to send frame" << err << std::endl;
        return;
    }
    // AV_TIME_BASE;
    AVPacket pkt;
    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;
    pkt.flags |= AV_PKT_FLAG_KEY;
    if (avcodec_receive_packet(video_context, &pkt) == 0) {
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

    float *audio_p = (float *)audio_frame->data[0];

//  AUDIO

        //  We convert to 44KHz
    for (int i=0;i!=735;i++)
        audio_44[i] = (*(snd.begin()+(int)(i/735.0*370))-128.0)/128;


        //  How many samples left to send?
    if (audio_pos+735>=1024)
    {
            //  Fill the audio out buffer
        memcpy( audio_p+audio_pos, audio_44, (1024-audio_pos)*sizeof(float) );

            //  Send to encoder
        audio_frame->pts = audio_frame_counter*1024;
        audio_frame_counter++;

        /* send the frame for encoding */
        err = avcodec_send_frame( audio_context, audio_frame);
        if (err < 0)
            throw "Error sending the frame to the encoder";

        err = avcodec_receive_packet( audio_context, &pkt);
        if (err == AVERROR(EAGAIN) || err == AVERROR_EOF)
            return;
        else if (err < 0)
            throw "Error encoding audio frame";

        pkt.stream_index = 1;
        av_interleaved_write_frame( ofctx, &pkt );

        av_packet_unref(&pkt);

            //  Fill the audio start
        memcpy( audio_p, audio_44+1024-audio_pos, (735-1024+audio_pos)*sizeof(float) );
        audio_pos = 735-1024+audio_pos;
    }
    else
    {
        memcpy( audio_p+audio_pos, audio_44, 735*sizeof(float) );
        audio_pos += 735;
    }

    frameCounter++;
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

    AVCodec* video_codec = nullptr;
    video_codec = avcodec_find_encoder(oformat->video_codec);
    if (!video_codec)
    {
        throw "Can't create video codec";
    }

    AVStream* stream = avformat_new_stream(ofctx, video_codec);
    if (!stream)
    {
        std::cout << "can't find format" << std::endl;
    }

    video_context = avcodec_alloc_context3(video_codec);

    if (!video_context)
    {
        throw "Can't create video codec context";
    }

    stream->codecpar->codec_id = oformat->video_codec;
    stream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
    stream->codecpar->width = 512;
    stream->codecpar->height = 342;
    stream->codecpar->format = AV_PIX_FMT_YUV420P;
    stream->codecpar->bit_rate = 60 * 6000 * 8;
    avcodec_parameters_to_context( video_context, stream->codecpar );
    video_context->time_base = (AVRational){ 1, 30 };
    video_context->max_b_frames = 2;
    video_context->gop_size = 12;
    video_context->framerate = (AVRational){ 60, 1 };

    //must remove the following
    // cctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    if (stream->codecpar->codec_id == AV_CODEC_ID_H264)
    {
        av_opt_set(video_context, "preset", "ultrafast", 0);
    }
    else if (stream->codecpar->codec_id == AV_CODEC_ID_H265)
    {
        av_opt_set(video_context, "preset", "ultrafast", 0);
    }

    avcodec_parameters_from_context( stream->codecpar, video_context );

    if ((err = avcodec_open2( video_context, video_codec, NULL )) < 0)
    {
        throw "Failed to open codec";
    }

// AUDIO
    /* find the MP2 encoder */
    // auto audio_codec = avcodec_find_encoder(AV_CODEC_ID_PCM_U8);
    // if (!audio_codec)
    //     throw "Audio codec not found";

    auto audio_codec = avcodec_find_encoder(oformat->audio_codec);
    if (!audio_codec)
        throw "Audio codec not found";

    audio_context = avcodec_alloc_context3(audio_codec);
    if (!audio_context)
        throw "Could not allocate audio codec context";

    /* put sample parameters */
    // audio_context->bit_rate = 22200;

    /* check that the encoder supports u8 pcm input */
    audio_context->sample_fmt = AV_SAMPLE_FMT_FLTP;
    if (!check_sample_fmt(audio_codec, audio_context->sample_fmt))
        throw "Encoder does not support FLT planar samples";
                // av_get_sample_fmt_name(c-planarsample_fmt));



//    audio_context->bit_rate = 22200*8;
    audio_context->sample_rate = 44100;
    audio_context->channel_layout = AV_CH_LAYOUT_MONO;
    audio_context->channels       = 1;

    /* open it */
    if (avcodec_open2( audio_context, audio_codec, NULL ) < 0)
        throw "Could not open audio codec";

    //  argh, avcodec_open2 set the frame_size to 1024!
    // audio_context->frame_size     = 735;


    AVStream* audio_stream = avformat_new_stream( ofctx, audio_codec );
    if (!audio_stream)
    {
        throw "Cannot create audio stream";
    }


    audio_stream->id = 1;   //  ???

    audio_stream->time_base = (AVRational){ 1, 1 };
    avcodec_parameters_from_context( audio_stream->codecpar, audio_context );








    audio_frame = av_frame_alloc();
    if (!audio_frame)
       throw "Error allocating an audio frame";

    audio_frame->format = audio_context->sample_fmt;
    audio_frame->channel_layout = audio_context->channel_layout;
    audio_frame->sample_rate = audio_context->sample_rate;
    audio_frame->nb_samples = 1024;      //  44100/60
    err = av_frame_get_buffer(audio_frame, 0);
    if (err < 0)
    {
        throw "Error allocating an audio buffer";
    }

    std::clog << "Line size = " << audio_frame->linesize[0] << "\n";
    std::clog << "Frame size = " << audio_context->frame_size << "\n";
     

// exit(0);














































    if (!(oformat->flags & AVFMT_NOFILE))
    {
        if ((err = avio_open(&ofctx->pb, filename.c_str(), AVIO_FLAG_WRITE)) < 0)
        {
            throw "Failed to open file";
        }
    }

    if ((err = avformat_write_header(ofctx, NULL)) < 0)
    {
        char buffer[1025];
        av_strerror( err, buffer, 1024 );
        std::clog << err << ":" << buffer << "\n";

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
        avcodec_send_frame(video_context, NULL);
        if (avcodec_receive_packet(video_context, &pkt) == 0) {
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
    if (audio_frame)
    {
        av_frame_free(&audio_frame);
    }
    if (video_context)
    {
        avcodec_free_context(&video_context);
    }
    if (audio_context)
    {
        avcodec_free_context(&audio_context);
    }
    if (ofctx)
    {
        avformat_free_context(ofctx);
    }

    std::clog << "#### End of video stream\n";
}
    virtual void write_frame( const image& img, const sound_frame_t &snd )
    {
        pushFrame( img, snd );
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
