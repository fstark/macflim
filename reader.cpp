#include "reader.hpp"

extern "C"{
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
}

class ffmpeg_reader : public input_reader
{
    AVFormatContext *format_context_;
    AVCodec *video_decoder_;
    AVCodec *audio_decoder_;
    AVStream *video_stream_;
    AVStream *audio_stream_;

    AVCodecContext *video_codec_context_;

    uint8_t *video_dst_data_[4] = {NULL};
    int video_dst_linesize_[4];

    AVPacket pkt_;
    AVFrame *frame_;

    int ixv;    //  Video frame index

    int video_frame_count = 0;

    std::unique_ptr<image> video_image_;        //  Size of the video input
    std::unique_ptr<image> default_image_;      //  Size of our output

    std::vector<image> images_;

    int image_ix = -1;

    double first_frame_second_;
    size_t frame_to_extract_;

    int decode_packet(int *got_frame, int cached, AVPacket &pkt)
    {
        int ret = 0;
        int decoded = pkt.size;
        *got_frame = 0;
        if (pkt.stream_index == ixv)
        {
            /* decode video frame */
            ret = avcodec_decode_video2( video_codec_context_, frame_, got_frame, &pkt );
            if (ret < 0) {
                // fprintf(stderr, "Error decoding video frame (%s)\n", av_err2str(ret));
                throw "VIDEO FRAME DECODING ERROR";
            }
            if (*got_frame)
            {
                // if (frame->width != width || frame->height != height ||
                //     frame->format != pix_fmt)
                //         throw "VIDEO FRAME DEFINITION CHANGED";     //  We could handle that by changing the image
                /* copy decoded frame to destination buffer:
                * this is required since rawvideo expects non aligned data */

                if (frame_->pts*av_q2d(video_stream_->time_base)>=first_frame_second_ && images_.size()<=video_frame_count)
                {
                    // printf("video_frame%s n:%d coded_n:%d presentation_ts:%ld / %f\n",
                    //     cached ? "(cached)" : "",
                    //     video_frame_count, frame_->coded_picture_number, frame_->pts, frame_->pts*av_q2d(video_stream_->time_base) );
                    video_frame_count++;
                    std::clog << "*" << std::flush;

                    av_image_copy(
                        video_dst_data_, video_dst_linesize_,
                                (const uint8_t **)(frame_->data), frame_->linesize,
                                video_codec_context_->pix_fmt, video_codec_context_->width, video_codec_context_->height );

                    // copy into image
                    video_image_->set_luma( video_dst_data_[0] );

                    images_.push_back( *default_image_ );
                    copy( images_.back(), *video_image_ );
                }
                else
                    std::clog << "." << std::flush;

                // images_[video_frame_count].set_luma( video_dst_data_[0] );
            }
        }

        extern bool sDebug;
        if (sDebug)
            std::clog << "decode_packet returing " << decoded << "\n";

        return decoded;
    }

public:
    ffmpeg_reader() {}

    ffmpeg_reader( const std::string &movie_path, double from, double duration )
    {
        std::clog << "avformat_open_input\n";

        //open video file
        if (avformat_open_input(&format_context_, movie_path.c_str(), NULL, NULL) != 0)
            throw "Cannot open input file";

        std::clog << "avformat_find_stream_info\n";

        //get stream info
        if (avformat_find_stream_info(format_context_, NULL) < 0)
            throw "Cannot find stream information";

        // dump the whole thing like ffprobe does
        //av_dump_format(pFormatCtx, 0, argv[1], 0);

        std::clog << "Searching for audio and video in " << format_context_->nb_streams << " streams\n";

        ixv = av_find_best_stream( format_context_,
                        AVMEDIA_TYPE_VIDEO,
                        -1,
                        -1,
                        &video_decoder_,
                        0 );
        auto ixa = av_find_best_stream( format_context_,
                        AVMEDIA_TYPE_AUDIO,
                        -1,
                        -1,
                        &audio_decoder_,
                        0 );

        if (ixv==AVERROR_STREAM_NOT_FOUND)
        {
            throw "NO VIDEO IN FILE\n";
        }
        if (ixv==AVERROR_DECODER_NOT_FOUND)
        {
            throw "NO SUITABLE VIDEO DECODER AVAILABLE\n";
        }
        if (ixa==AVERROR_STREAM_NOT_FOUND)
        {
            throw "NO AUDIO IN FILE\n";
        }
        if (ixa==AVERROR_DECODER_NOT_FOUND)
        {
            throw "NO SUITABLE AUDIO DECODER AVAILABLE\n";
        }

        double seek_to = std::max(from-5.0,0.0);    //  We seek to 5 seconds earlier, if we can
        if (avformat_seek_file( format_context_, -1, seek_to*AV_TIME_BASE, seek_to*AV_TIME_BASE, seek_to*AV_TIME_BASE, AVSEEK_FLAG_ANY )<0)
            throw "CANNOT SEEK IN FILE\n";

        video_stream_ = format_context_->streams[ixv];
        audio_stream_ = format_context_->streams[ixa];

        // get the frame rate of each stream

        std::clog   << "Video : "
                    << " " << video_stream_->codecpar->width << "x" << video_stream_->codecpar->height
                    << "@" << av_q2d( video_stream_->r_frame_rate )
                    << " fps" 
                    << " timebase:" 
                    << av_q2d(video_stream_->time_base)
                    << "\n";
        std::clog   << "Audio : "
                    << " " << audio_stream_->codecpar->sample_rate
                    << "Hz \n";

        first_frame_second_ = from;
        frame_to_extract_ = duration*av_q2d( video_stream_->r_frame_rate );

//      non needed, the decoder was found by av_find_best_stream
//        dec = avcodec_find_decoder( video_stream_->codecpar->codec_id );

            //  allocate the context
        video_codec_context_ = avcodec_alloc_context3( video_decoder_ );
        if (!video_codec_context_)
            throw "CANNOT ALLOCATE CONTEXT\n";

            //  copy the parameters
        if (avcodec_parameters_to_context( video_codec_context_, video_stream_->codecpar ) < 0)
            throw "FAILED TO COPY PARAMETERS\n";

        AVDictionary *opts = NULL;

        av_dict_set(&opts, "refcounted_frames", "0", 0);    //  Do not refcount

        if (avcodec_open2( video_codec_context_, video_decoder_, &opts ) < 0)
            throw "CANNOT OPEN VIDEO CODEC\n";

        std::clog << "VIDEO CODEC OPENED WITH PIXEL FORMAT " << av_get_pix_fmt_name(video_codec_context_->pix_fmt) << "\n";

        if (video_codec_context_->pix_fmt!=AV_PIX_FMT_YUV420P)
            throw "WAS EXPECTING A YUV240P PIXEL FORMAT";

        auto bufsize = av_image_alloc(
            video_dst_data_,
            video_dst_linesize_,
            video_codec_context_->width,
            video_codec_context_->height,
            video_codec_context_->pix_fmt,
            1 );

        if (bufsize<0)
            throw "CANNOT ALLOCATE IMAGE\n";

        video_image_ = std::make_unique<image>( video_codec_context_->width, video_codec_context_->height );

        double aspect = video_codec_context_->width/(double)video_codec_context_->height;
        if (aspect>512/342.0)
        {
            default_image_ = std::make_unique<image>( 342*aspect, 342 );
        }
        else
        {
            default_image_ = std::make_unique<image>( 512, 512/aspect );
        }

        std::clog << "Image structure:\n";
        std::clog << video_dst_linesize_[0] << " " << video_dst_linesize_[1] << " " << video_dst_linesize_[2] << " " << video_dst_linesize_[3] << "\n";
        std::clog << (video_dst_linesize_[0] + video_dst_linesize_[1] + video_dst_linesize_[2] + video_dst_linesize_[3])*video_codec_context_->height << "\n";
        std::clog << bufsize << "\n";

        std::clog << video_dst_data_[1]-video_dst_data_[0] << " " << video_dst_data_[2]-video_dst_data_[1] << " " << video_dst_data_[3]-video_dst_data_[2] << "\n";

        frame_ = av_frame_alloc();

            //  Ready to read images
        av_init_packet(&pkt_);
        pkt_.data = NULL;
        pkt_.size = 0;

        int got_frame = 0;
        bool abort = false;



        while (av_read_frame(format_context_, &pkt_) >= 0)
        {
            AVPacket orig_pkt = pkt_;
            do {
                auto ret = decode_packet( &got_frame, 0, pkt_ );
                if (images_.size()==frame_to_extract_)
                    goto end;
                if (ret < 0)
                    break;
                pkt_.data += ret;
                pkt_.size -= ret;
            } while (pkt_.size > 0);
        }
        /* flush cached frames */
        if (images_.size()!=frame_to_extract_)
        {
            pkt_.data = NULL;
            pkt_.size = 0;
            do {
                decode_packet(&got_frame, 1, pkt_);
            } while (got_frame);
        }

end:

        image_ix = 0;

        std::clog << "\n";
    }

    ~ffmpeg_reader()
    {
        avformat_close_input( &format_context_);
        std::clog << "Closed media file\n";
    }

    virtual double frame_rate()
    {
        return av_q2d( video_stream_->r_frame_rate );
    }

    virtual std::unique_ptr<image> next()
    {
        if (image_ix==-1)
            return nullptr;
        if (image_ix==images_.size())
            return nullptr;
        auto res = std::make_unique<image>( images_[image_ix] );
        image_ix++;
        return res;
    }

    virtual size_t sample_rate() { return 60*370; }
    virtual std::vector<double> raw_sound() { return {}; }
};

std::unique_ptr<input_reader> make_ffmpeg_reader( const std::string &movie_path, double from, double to )
{
    try
    {
        return std::make_unique<ffmpeg_reader>( movie_path, from, to );
    }
    catch (const char *e)
    {
        std::clog << "**** ERROR : " << e << "\n";
        return nullptr;
    }
}
