#include "reader.hpp"

extern "C"{
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
}

#include <array>

/// This stores a sound buffer and transform it into a suitable format for flims
class sound_buffer
{
    std::vector<float> data_;

    size_t channel_count_ = 0;      //  # of channels
    size_t sample_rate_ = 0;        //  # of samples per second
public:
    sound_buffer( size_t channel_count, size_t sample_rate ) :
        channel_count_{channel_count},
        sample_rate_ {sample_rate}
    {}

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

    class sound_frame
    {
        static const size_t size = 370;
        std::array<uint8_t,size> data_;

        public:
            sound_frame()
            {
                for (int i=0;i!=size;i++)
                    data_[i] = 128;
            }

            template <typename T, typename U> sound_frame( T from, U min, U max )
            {
                for (int i=0;i!=size;i++)
                    data_[i] = ((double)(*from++)-min)/(max-min)*255;
            }

            template <typename T> sound_frame( T from )
            {
                for (int i=0;i!=size;i++)
                    data_[i] = *from++;
            }
    };

    //  Extract a certain number of 370 bytes frames of 1/60th of a second
    std::vector<uint8_t> extract( size_t frame_count )
    {
        //  WIP
        //  First, we move the to the "correct" sample rate

        // std::vector<float> resampled;

        // size_t out_sample_rate = 22200; //  370*60

        // double in_duration = sound_buffer.size()/(double)sample_rate_;

        // for (int i=0;sample_count;i++)

        // auto min_sample = *std::min_element( std::begin(data_), std::end(data_) );
        // auto max_sample = *std::max_element( std::begin(data_), std::end(data_) );
    }
};

class ffmpeg_reader : public input_reader
{
    AVFormatContext *format_context_;
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
        else if (pkt.stream_index == ixa)
        {
            ret = avcodec_decode_audio4(audio_codec_context_, frame_, got_frame, &pkt);
            if (ret < 0)
            {
                fprintf(stderr, "Error decoding audio frame (%s)\n", av_err2str(ret));
                return ret;
            }
          /* Some audio decoders decode only part of the packet, and have to be
           * called again with the remainder of the packet data.
           * Sample: fate-suite/lossless-audio/luckynight-partial.shn
           * Also, some decoders might over-read the packet. */
          decoded = FFMIN(ret, pkt.size);
  

 static int audio_frame_count = 0; 

          if (*got_frame) {
              size_t unpadded_linesize = frame_->nb_samples * av_get_bytes_per_sample((AVSampleFormat)frame_->format);
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
            throw "NO AUDIO IN FILE\n";
        }
        if (ixa==AVERROR_DECODER_NOT_FOUND)
        {
            throw "NO SUITABLE AUDIO DECODER AVAILABLE\n";
        }

        std::clog << "Video stream index :" << ixv << "\n";
        std::clog << "Audio stream index :" << ixa << "\n";

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
            throw "CANNOT ALLOCATE VIDEO CODEC CONTEXT\n";

            //  copy the parameters
        if (avcodec_parameters_to_context( video_codec_context_, video_stream_->codecpar ) < 0)
            throw "FAILED TO COPY VIDEO CODEC PARAMETERS\n";

        audio_codec_context_ = avcodec_alloc_context3( audio_decoder_ );
        // audio_codec_context_ = format_context_->streams[ixa]->codec;
        if (!audio_codec_context_)
            throw "CANNOT ALLOCATE AUDIO CODEC CONTEXT\n";

        if (avcodec_parameters_to_context( audio_codec_context_, audio_stream_->codecpar ) < 0)
            throw "FAILED TO COPY AUDIO CODEC PARAMETERS\n";

        AVDictionary *opts = NULL;

        av_dict_set(&opts, "refcounted_frames", "0", 0);    //  Do not refcount

        if (avcodec_open2( video_codec_context_, video_decoder_, &opts ) < 0)
            throw "CANNOT OPEN VIDEO CODEC\n";
        if (avcodec_open2( audio_codec_context_, audio_decoder_, nullptr ) < 0)
            throw "CANNOT OPEN AUDIO CODEC\n";

std::clog << "AUDIO CODEC: " << avcodec_get_name( audio_codec_context_->codec_id ) << "\n";

    AVSampleFormat sfmt = audio_codec_context_->sample_fmt;
    int n_channels = audio_codec_context_->channels;

std::clog << "SAMPLE FORMAT:" << av_get_sample_fmt_name( sfmt ) << "\n";
std::clog << "# CHANNELS   :" << n_channels << "\n";
std::clog << "PLANAR       :" << (av_sample_fmt_is_planar(sfmt)?"YES":"NO") << "\n";

    if (av_sample_fmt_is_planar(sfmt))
    {
    //          const char *packed = av_get_sample_fmt_name(sfmt);
    //          printf("Warning: the sample format the decoder produced is planar "
    //                 "(%s). This example will output the first channel only.\n",
    //                 packed ? packed : "?");
             sfmt = av_get_packed_sample_fmt(sfmt);
std::clog << "PACKED FORMAT:" << av_get_sample_fmt_name( sfmt ) << "\n";

std::clog << "SAMPLE RATE  :" << audio_codec_context_->sample_rate << "\n";

    //          n_channels = 1;
    }
    sound_ = std::make_unique<sound_buffer>( n_channels, audio_codec_context_->sample_rate );

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
