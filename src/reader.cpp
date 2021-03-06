#include "reader.hpp"

extern "C"{
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
}

#include <array>

extern bool sDebug;

/// This stores a sound buffer and transform it into a suitable format for flims
class sound_buffer
{
    std::vector<float> data_;

    size_t channel_count_ = 0;      //  # of channels
    size_t sample_rate_ = 0;        //  # of samples per second

    float min_sample_;
    float max_sample_;

public:
    sound_buffer( size_t channel_count, size_t sample_rate ) :
        channel_count_{channel_count},
        sample_rate_ {sample_rate}
    {}

    //  Append silence
    void append_silence( float duration )
    {
        size_t sample_count = sample_rate_ * duration;
        for (int i=0;i!=sample_count;i++)
        {
            data_.push_back( 0 );
        }
    }

    //  Append sample_count samples over each of the channels
    void append_samples( float **samples, size_t sample_count )
    {
        for (int i=0;i!=sample_count;i++)
        {
            float v = 0;
            for (int j=0;j!=channel_count_;j++)
                v += samples[j][i];

            data_.push_back( v/channel_count_ );
        }
    }

    void process()
    {
        min_sample_ = *std::min_element( std::begin(data_), std::end(data_) );
        max_sample_ = *std::max_element( std::begin(data_), std::end(data_) );
    }

    //  Extract a 370 bytes frames (1/60th of a second)
    std::unique_ptr<sound_frame_t> extract( size_t frame )
    {
        double t = frame / 60.0;        //  Time in seconds
        size_t start = t * sample_rate_;

        if (start>=data_.size())
            return nullptr;

        auto fr = std::make_unique<sound_frame_t>();

        for (int i=0;i!=sound_frame_t::size;i++)
        {
            size_t index = start+(i/370.0/60.0)*sample_rate_;
            if (index<data_.size())
                fr->at(i) = (data_[index]-min_sample_)/(max_sample_-min_sample_)*255;
            else
                fr->at(i) = 128;
        }

        return fr;
    }
};

class ffmpeg_reader : public input_reader
{
    AVFormatContext *format_context_ = nullptr;
    AVCodec *video_decoder_;
    AVCodec *audio_decoder_;
    AVStream *video_stream_;
    AVStream *audio_stream_;

    AVCodecContext *video_codec_context_;
    AVCodecContext *audio_codec_context_;

    uint8_t *video_dst_data_[4] = {NULL};
    int video_dst_linesize_[4];

    AVPacket pkt_;
    AVFrame *frame_;

    int ixv;    //  Video frame index
    int ixa;    //  Audio frame index

    int video_frame_count = 0;

    std::unique_ptr<image> video_image_;        //  Size of the video input
    std::unique_ptr<image> default_image_;      //  Size of our output

    std::vector<image> images_;

    std::unique_ptr<sound_buffer> sound_;

    int image_ix = -1;
    int sound_ix = -1;

    double first_frame_second_;
    size_t frame_to_extract_;

    bool found_sound_ = false;                   //  To track if sounds starts with an offset

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
#define noVERBOSE

#ifdef VERBOSE
    std::clog << "*" << std::flush;
#endif

            if (*got_frame)
            {
                // if (frame->width != width || frame->height != height ||
                //     frame->format != pix_fmt)
                //         throw "VIDEO FRAME DEFINITION CHANGED";     //  We could handle that by changing the image
                /* copy decoded frame to destination buffer:
                * this is required since rawvideo expects non aligned data */
#ifdef VERBOSE
    std::clog << "VIDEO FRAME TS = " << frame_->pts*av_q2d(video_stream_->time_base) << "\n";
#endif

                if (frame_->pts*av_q2d(video_stream_->time_base)>=first_frame_second_ && images_.size()<=video_frame_count)
                {
#ifdef VERBOSE
                    printf("video_frame%s n:%d coded_n:%d presentation_ts:%ld / %f\n",
                        cached ? "(cached)" : "",
                        video_frame_count, frame_->coded_picture_number, frame_->pts, frame_->pts*av_q2d(video_stream_->time_base) );
#endif
                    video_frame_count++;
                    std::clog << "Read " << video_frame_count << " frames\r" << std::flush;

                    av_image_copy(
                        video_dst_data_, video_dst_linesize_,
                                (const uint8_t **)(frame_->data), frame_->linesize,
                                video_codec_context_->pix_fmt, video_codec_context_->width, video_codec_context_->height );

                    // copy into image
                    video_image_->set_luma( video_dst_data_[0] );

                    images_.push_back( *default_image_ );
                    copy( images_.back(), *video_image_ );

                    // write_image( "/tmp/dump.pgm", *video_image_ );
                }
#ifdef VERBOSE
                else
                    std::clog << "." << std::flush;  //  We are skipping frames
#endif

                // images_[video_frame_count].set_luma( video_dst_data_[0] );
            }
        }
        else if (pkt.stream_index == ixa)
        {
#if 0
std::clog << "avcodec_receive_frame => ";
        ret = avcodec_receive_frame(audio_codec_context_,frame_);
std::clog << ret << "\n";
        if (ret == 0)
            *got_frame = true;
        if (ret == AVERROR(EAGAIN))
            return 0;
        if (ret == 0)
        {
            std::clog << "avcodec_send_packet => ";
            ret = avcodec_send_packet(audio_codec_context_, &pkt);
            std::clog << ret << "\n";
        }
        if (ret == AVERROR(EAGAIN))
            return 0;
#else
            ret = avcodec_decode_audio4(audio_codec_context_, frame_, got_frame, &pkt);
            if (ret < 0)
            {
//                auto s = av_err2str(ret);
                throw "AUDIO FRAME DECODING ERROR";
            }
#endif
          /* Some audio decoders decode only part of the packet, and have to be
           * called again with the remainder of the packet data.
           * Sample: fate-suite/lossless-audio/luckynight-partial.shn
           * Also, some decoders might over-read the packet. */
          decoded = FFMIN(ret, pkt.size);
  

 static int audio_frame_count = 0; 

          if (*got_frame)
          {
#ifdef VERBOSE
            std::clog << "AUDIO: " << frame_->pts*av_q2d(audio_stream_->time_base) << " sample count " << frame_->nb_samples << "\n";
#endif
            if (frame_->pts*av_q2d(audio_stream_->time_base)>=first_frame_second_)
            {
#ifdef VERBOSE
            std::clog << "USING AUDIO FRAME\n";
#endif

            if (!found_sound_)
            {
                found_sound_ = true;
                auto skip = frame_->pts*av_q2d(audio_stream_->time_base)-first_frame_second_;
                if (skip>0)
                {
                    std::clog << "Inserting " << skip << " seconds of silence\n";
                    sound_->append_silence( skip );
                }
            }

            //   size_t unpadded_linesize = frame_->nb_samples * av_get_bytes_per_sample((AVSampleFormat)frame_->format);
            //   printf("audio_frame%s n:%d nb_samples:%d pts:%s\n",
            //          cached ? "(cached)" : "",
            //          audio_frame_count++, frame_->nb_samples,
            //          av_ts2timestr(frame_->pts, &audio_codec_context_->time_base));
  
              /* Write the raw audio data samples of the first plane. This works
               * fine for packed formats (e.g. AV_SAMPLE_FMT_S16). However,
               * most audio decoders output planar audio, which uses a separate
               * plane of audio samples for each channel (e.g. AV_SAMPLE_FMT_S16P).
               * In other words, this code will write only the first audio channel
               * in these cases.
               * You should use libswresample or libavfilter to convert the frame
               * to packed data. */
            //   fwrite(frame->extended_data[0], 1, unpadded_linesize, audio_dst_file);

// std::clog << frame_->nb_samples << " (" << unpadded_linesize << " bytes) :";

// for (int i=0;i!=frame_->nb_samples;i++)
// {
//     float v = 0;
//     for (int j=0;j!=audio_codec_context_->channels;j++)
//         v += ((float *)frame_->extended_data[j])[i];

//     std::clog << v/audio_codec_context_->channels << " ";
// }
// std::clog << "\n";

            sound_->append_samples( (float **)frame_->extended_data, frame_->nb_samples );
          }
        //   else
        //     std::clog << "." << std::flush;
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

        av_log_set_level( AV_LOG_WARNING );

        //open video file
        if (avformat_open_input(&format_context_, movie_path.c_str(), NULL, NULL) != 0)
            throw "Cannot open input file";

        //get stream info
        if (avformat_find_stream_info(format_context_, NULL) < 0)
            throw "Cannot find stream information";

        // dump the whole thing like ffprobe does
        //av_dump_format(pFormatCtx, 0, argv[1], 0);

        if (sDebug)
            std::clog << "Searching for audio and video in " << format_context_->nb_streams << " streams\n";

        ixv = av_find_best_stream( format_context_,
                        AVMEDIA_TYPE_VIDEO,
                        -1,
                        -1,
                        &video_decoder_,
                        0 );
        ixa = av_find_best_stream( format_context_,
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
            std::cerr << "NO SOUND -- INSERTING SILENCE";
        }
        if (ixa==AVERROR_DECODER_NOT_FOUND)
        {
            throw "NO SUITABLE AUDIO DECODER AVAILABLE\n";
        }

        if (sDebug)
        {
            std::clog << "Video stream index :" << ixv << "\n";
            std::clog << "Audio stream index :" << ixa << "\n";
        }

        double seek_to = std::max(from-10.0,0.0);    //  We seek to 10 seconds earlier, if we can
        if (avformat_seek_file( format_context_, -1, seek_to*AV_TIME_BASE, seek_to*AV_TIME_BASE, seek_to*AV_TIME_BASE, AVSEEK_FLAG_ANY )<0)
            throw "CANNOT SEEK IN FILE\n";
        // if (avformat_seek_file( format_context_, ixv, 0, 0, 0, AVSEEK_FLAG_FRAME )<0)
        //     throw "CANNOT SEEK IN FILE\n";

        video_stream_ = format_context_->streams[ixv];
        audio_stream_ = ixa!=AVERROR_STREAM_NOT_FOUND?format_context_->streams[ixa]:nullptr;

        // get the frame rate of each stream

        if (sDebug)
        {
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
        }

        first_frame_second_ = from;
        frame_to_extract_ = duration*av_q2d( video_stream_->r_frame_rate );

//      non needed, the decoder was found by av_find_best_stream
//        dec = avcodec_find_decoder( video_stream_->codecpar->codec_id );

            //  allocate the context
        video_codec_context_ = avcodec_alloc_context3( video_decoder_ );
        if (!video_codec_context_)
            throw "CANNOT ALLOCATE VIDEO CODEC CONTEXT\n";

            //  copy the parameters
        if (avcodec_parameters_to_context( video_codec_context_, video_stream_->codecpar ) < 0)
            throw "FAILED TO COPY VIDEO CODEC PARAMETERS\n";

        if (ixa!=AVERROR_STREAM_NOT_FOUND)
        {
            audio_codec_context_ = avcodec_alloc_context3( audio_decoder_ );
            // audio_codec_context_ = format_context_->streams[ixa]->codec;
            if (!audio_codec_context_)
                throw "CANNOT ALLOCATE AUDIO CODEC CONTEXT\n";

            if (avcodec_parameters_to_context( audio_codec_context_, audio_stream_->codecpar ) < 0)
                throw "FAILED TO COPY AUDIO CODEC PARAMETERS\n";
        }

        AVDictionary *opts = NULL;

        av_dict_set(&opts, "refcounted_frames", "0", 0);    //  Do not refcount

        if (avcodec_open2( video_codec_context_, video_decoder_, &opts ) < 0)
            throw "CANNOT OPEN VIDEO CODEC\n";
        if (ixa!=AVERROR_STREAM_NOT_FOUND)
        {
            if (avcodec_open2( audio_codec_context_, audio_decoder_, nullptr ) < 0)
                throw "CANNOT OPEN AUDIO CODEC\n";
            if (sDebug)
                std::clog << "AUDIO CODEC: " << avcodec_get_name( audio_codec_context_->codec_id ) << "\n";

            AVSampleFormat sfmt = audio_codec_context_->sample_fmt;
            int n_channels = audio_codec_context_->channels;

                if (sDebug)
                {
                    std::clog << "SAMPLE FORMAT:" << av_get_sample_fmt_name( sfmt ) << "\n";
                    std::clog << "# CHANNELS   :" << n_channels << "\n";
                    std::clog << "PLANAR       :" << (av_sample_fmt_is_planar(sfmt)?"YES":"NO") << "\n";
                }

            if (av_sample_fmt_is_planar(sfmt))
            {
            //          const char *packed = av_get_sample_fmt_name(sfmt);
            //          printf("Warning: the sample format the decoder produced is planar "
            //                 "(%s). This example will output the first channel only.\n",
            //                 packed ? packed : "?");
                    sfmt = av_get_packed_sample_fmt(sfmt);

                if (sDebug)
                {
                    std::clog << "PACKED FORMAT:" << av_get_sample_fmt_name( sfmt ) << "\n";
                    std::clog << "SAMPLE RATE  :" << audio_codec_context_->sample_rate << "\n";
                }
            //          n_channels = 1;
            }
            sound_ = std::make_unique<sound_buffer>( n_channels, audio_codec_context_->sample_rate );

        }
        else
            sound_ = nullptr;


        if (sDebug)
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

        if (sDebug)
        {
            std::clog << "Image structure:\n";
            std::clog << video_dst_linesize_[0] << " " << video_dst_linesize_[1] << " " << video_dst_linesize_[2] << " " << video_dst_linesize_[3] << "\n";
            std::clog << (video_dst_linesize_[0] + video_dst_linesize_[1] + video_dst_linesize_[2] + video_dst_linesize_[3])*video_codec_context_->height << "\n";
            std::clog << bufsize << "\n";
        }

        // std::clog << video_dst_data_[1]-video_dst_data_[0] << " " << video_dst_data_[2]-video_dst_data_[1] << " " << video_dst_data_[3]-video_dst_data_[2] << "\n";

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

        std::clog << "\n";

        image_ix = 0;

        if (sDebug)
            std::clog << "\nAcquired " << images_.size() << " frames of video for a total of " << images_.size()/av_q2d( video_stream_->r_frame_rate ) << " seconds \n";

        if (ixa!=AVERROR_STREAM_NOT_FOUND)
        {
            sound_->process();
            sound_ix = 0;
        }

        if (sDebug)
            std::clog << "\n";
    }

    ~ffmpeg_reader()
    {
        avformat_close_input( &format_context_);
        if (sDebug)
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

    virtual std::unique_ptr<sound_frame_t> next_sound()
    {
        if (ixa!=AVERROR_STREAM_NOT_FOUND)
        {
            auto s = std::make_unique<sound_frame_t>();
            return sound_->extract( sound_ix++ );
        }
        return nullptr;
    }
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
