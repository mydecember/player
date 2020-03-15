//
// Created by zz on 19-11-14.
//

#include "Demuxer.h"

#include<mutex>
#include<thread>
#include "libyuv.h"
#include "base/utils.h"
#include "UERead.h"

#define LOG_BUF_PREFIX_SIZE 512
#define LOG_BUF_SIZE 2048
static char logBufPrefix[LOG_BUF_PREFIX_SIZE];
static char logBuffer[LOG_BUF_SIZE];
static pthread_mutex_t cb_av_log_lock;

char static SliceType(int typeNum) {
    char c = 'N';
    switch(typeNum) {
        case 0:
        case 3:
        case 5:
        case 8:
            c = 'P';
            break;
        case 1:
        case 6:
            c = 'B';
            break;
        case 2:
        case 4:
        case 7:
        case 9:
            c = 'I';
            break;
        default:
            break;
    }
    return c;
}
void static checkSliceType(uint8_t *data, int len) {

    bs_t bst;
    int NRI = (data[4] >>5);
    int nalType = ((data[4]&0x1F));
    bs_init(&bst, data + 5, len - 5);
    bool firstMb = bs_read_ue(&bst);
    int sliceType = bs_read_ue(&bst);
    int ppsId =bs_read_ue(&bst);
    Log(" ===== nalType %d first mb %d sliceType %d:%c ppsId %d",nalType, firstMb, sliceType,SliceType(sliceType), ppsId);
}

static void log_callback_null(void *ptr, int level, const char *fmt, va_list vl){
    if (level  > AV_LOG_ERROR)
        return;

    int cnt;
    pthread_mutex_lock(&cb_av_log_lock);
    cnt = snprintf(logBufPrefix, LOG_BUF_PREFIX_SIZE, "%s", fmt);
    cnt = vsnprintf(logBuffer, LOG_BUF_SIZE, logBufPrefix, vl);
    Log(" ffmpeg %s", logBuffer);
    pthread_mutex_unlock(&cb_av_log_lock);
}


//using namespace media_ffmpeg;
using namespace std;
static once_flag g_init_flag;
static  void Register() {

/* END */
    av_log_set_level(AV_LOG_ERROR);
    av_log_set_flags(AV_LOG_SKIP_REPEATED);
    av_log_set_callback(log_callback_null);

    av_register_all();

}

Demuxer::Demuxer() :videoStreamId_(-1),
                    audioStreamId_(-1),
                    av_fmt_ctx(NULL),
                    videoStream_(NULL),
                    audioStream_(NULL),
                    audioCodecCtx_(NULL),
                    videoCodecCtx_(NULL),
                    videoStreamEof_(true),
                    audioStreamEof_(true),
                    demuxerEnd_(true),
                    videoDecodeEof_(true),
                    audioDecodeEof_(true),
                    loop_(false){
    call_once(g_init_flag, Register);
    got_nums_ = 0;

}
Demuxer::~Demuxer() {

}
int Demuxer::Open(std::string inputFile, int streamType) {
    streamType_ = streamType;
    Log("to open %s", inputFile.c_str());
    AVDictionary *opts = NULL;// = NULL;


    if (av_stristart(inputFile.c_str(), "rtmp", NULL) ||
        av_stristart(inputFile.c_str(), "rtsp", NULL)) {
        // There is total different meaning for 'timeout' option in rtmp
        Log("remove 'timeout' option for rtmp.\n");
        av_dict_set(&opts, "timeout", NULL,0);
    }
    //av_dict_set_int(&opts, "skip-calc-frame-rate", 1, 0);

    int ret = avformat_open_input(&av_fmt_ctx, inputFile.c_str(), NULL, opts == NULL? NULL:&opts);
    if (ret != 0) {
        Log("open input error %d", ret);
        return -1;
    }
    if (avformat_find_stream_info(av_fmt_ctx, NULL) < 0) {
        Log("avformat_find_stream_info error");
        return -2;
    }

    for (unsigned int i = 0; i < av_fmt_ctx->nb_streams; ++i) {
        AVStream *stream = av_fmt_ctx->streams[i];
        auto ctx = av_fmt_ctx->streams[i]->codec;
        if (ctx->codec_type == AVMEDIA_TYPE_VIDEO && (streamType & 2))
        {
            videoStreamId_ = i;
            videoStreamEof_ = false;
            videoDecodeEof_ = false;
            //break;
        }
        // else if (ctx->codec_type == AVMEDIA_TYPE_AUDIO && !onlyDecVideo_)
        // {
        //     audioStreamId_ = i;
        // }

        stream->discard = AVDISCARD_ALL;
    }

    if (streamType & 1) {
        audioStreamId_ = av_find_best_stream(av_fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    }

    if (videoStreamId_ < 0 && audioStreamId_ < 0) {
        return -3;
    }

    if (videoStreamId_ >= 0) {
        //Get video stream and codec
        videoStream_ = av_fmt_ctx->streams[videoStreamId_];
        videoStream_->discard = AVDISCARD_DEFAULT;
        AVCodec* videoCodec_ = nullptr;
        videoCodecCtx_ = videoStream_->codec;

        Log("find video stream width:%d height:%d", videoCodecCtx_->width, videoCodecCtx_->height);

        videoCodec_ = avcodec_find_decoder(videoCodecCtx_->codec_id);

        //get frame rate for decoder
        videoCodecCtx_->framerate = videoStream_->avg_frame_rate;
        if (videoCodec_ == nullptr) {
            videoCodecCtx_ = nullptr;
            Log("Cannot find video codec");
            return -4;
        }
        videoCodecCtx_->thread_count = 2;
        //videoCodecCtx_->thread_type = FF_THREAD_FRAME;//FF_THREAD_SLICE;//FF_THREAD_FRAME;//1;
        if (avcodec_open2(videoCodecCtx_, videoCodec_, nullptr) < 0) {
            Log("Cannot open video codec");
            return -5;
        }
    }

    if (audioStreamId_ >= 0) {
        audioStreamEof_ = false;
        audioDecodeEof_ = false;
        audioStream_ = av_fmt_ctx->streams[audioStreamId_];
        audioStream_->discard = AVDISCARD_DEFAULT;
        AVCodec* audioCodec_ = nullptr;
        audioCodecCtx_ = audioStream_->codec;

        audioCodec_ = avcodec_find_decoder(audioCodecCtx_->codec_id);
        if (audioCodec_ == nullptr) {
            audioCodecCtx_ = nullptr;
            Log("Cannot find audio codec");
        }
        else if (avcodec_open2(audioCodecCtx_, audioCodec_, nullptr) < 0) {
            Log("Cannot open audio codec");
        }
        else {
            audioStream_->discard = AVDISCARD_DEFAULT;
        }
        audioFrame_.reset(av_frame_alloc());
    }
    videoFrame_.reset(av_frame_alloc());
    demuxerEnd_ = false;
    packet_.reset(new AVPacket());
    Log("open file ok video id %d audio id %d w:%d h%d", videoStreamId_, audioStreamId_,videoCodecCtx_->width, videoCodecCtx_->height);
    return ret;
}

int Demuxer::GetWidth() {
    return videoCodecCtx_->width;
}
int Demuxer::GetHeight() {
    return videoCodecCtx_->height;
}

int Demuxer::Close() {
    if (videoCodecCtx_)
    {
        avcodec_close(videoCodecCtx_);
        videoCodecCtx_ = nullptr;
    }

    if (audioCodecCtx_)
    {
        avcodec_close(audioCodecCtx_);
        audioCodecCtx_ = nullptr;
    }

    if(av_fmt_ctx)
    {
        avformat_close_input(&av_fmt_ctx);
        av_fmt_ctx= nullptr;
    }

    videoStreamId_ = -1;
    audioStreamId_ = -1;
    return 0;
}

void Demuxer::FlushDecoderBuffers() {
    if (videoCodecCtx_) {
        avcodec_flush_buffers(videoCodecCtx_);
    }
    if (audioCodecCtx_) {
        avcodec_flush_buffers(audioCodecCtx_);
    }
}

void Demuxer::FrameDataToVector(AVFrame* frame, bool isAudio) {

    if (isAudio) {
        int size = frame->linesize[0];
        if (audioData_.size() != size ) {
            audioData_.resize(size);
        }
        memcpy(audioData_.data(), frame->data[0], size);
    } else {
        int size = (frame->linesize[0] + frame->linesize[1] + frame->linesize[2]) * frame->height;
        //if
    }

}

int Demuxer::GetFrame(uint8_t** data, int& len , int &gotframe, int64_t& tm, AVFrame* result) {
    if (demuxerEnd_ || (audioDecodeEof_ && videoDecodeEof_)) {
        return -1;
    }
    // Only get video packet
        gotframe = 0;
        // Video Stream reached the end file.
        if (!videoDecodeEof_ && videoCodecCtx_ && videoStreamEof_) {
            packet_->data = NULL;
            packet_->size = 0;
            Log("zfq get video frame eof");
            avcodec_decode_video2(videoCodecCtx_, videoFrame_.get(), &gotframe, packet_.get());

            if (gotframe) {
                Log(" zfq get video frame1  used ");
                if (result) {
                    av_frame_ref(result, videoFrame_.get());
                }

                av_frame_unref(videoFrame_.get());
                //if (videoData_.size() != videoFrame_->linesize)
                //*data = videoFrame_->pkt_pts;
                return 0;
            } else {
                videoDecodeEof_ = true;
            }
        }
        if (!audioDecodeEof_ && audioCodecCtx_ && audioStreamEof_) {
            packet_->data = NULL;
            packet_->size = 0;
            avcodec_decode_audio4(audioCodecCtx_, audioFrame_.get(), &gotframe, packet_.get());
            if (gotframe) {
                Log("zfq get audio frame eof");

                av_frame_unref(audioFrame_.get());
                return 0;
            } else {
                audioDecodeEof_ = true;
            }
        }

//        if ((!videoCodecCtx_ && audioDecodeEof_) ||
//            (!audioCodecCtx_ && videoDecodeEof_) ||
//            (videoCodecCtx_ && videoDecodeEof_ && audioCodecCtx_ && audioDecodeEof_)) {
        if (audioDecodeEof_ && videoDecodeEof_) {
            if (loop_) {
                //avformat_seek_duration_file()
                //int avformat_seek_file(AVFormatContext *s, int stream_index, int64_t min_ts, int64_t ts, int64_t max_ts, int flags);
                int ret = av_seek_frame(av_fmt_ctx, -1, 0, 0);
                //int ret = avformat_seek_duration_file(av_fmt_ctx, -1, 0, 0);
                if (ret < 0) {
                    Log("play loop avformat_seek_duration_file to 0 error: %d", ret);
                }
                Log("play loop avformat_seek_duration_file to 0 sucess");
                if (videoCodecCtx_) {
                    videoStreamEof_ = false;
                    videoDecodeEof_ = false;

                }
                if (audioCodecCtx_) {
                    audioStreamEof_ = false;
                    audioDecodeEof_ = false;
                }

                FlushDecoderBuffers();
            } else {
                demuxerEnd_ = true;
                return -1;
            }
        }

        int ret = av_read_frame(av_fmt_ctx, packet_.get());
        if (ret < 0) {
            if(ret == AVERROR_EOF) {
                Log("av_read_frame AVERROR_EOF");
                audioStreamEof_ = true;
                videoStreamEof_ = true;
                return 0;
            }
            return -1;
        }
    //checkSliceType(packet_->data, packet_->size);
        if (packet_->stream_index == audioStreamId_)
        {
            do {
                int decodedsize = avcodec_decode_audio4(audioCodecCtx_, audioFrame_.get(), &gotframe, packet_.get());

                if (decodedsize <= 0 || gotframe <= 0) {
                    break;
                }

                // LOG(LS_INFO, TRIVAL_MODULE) << "decode audio frame pts " << audioFrame_->pts
                //                             << " pkt_pts " << audioFrame_->pkt_pts
                //                             << " pakcet pts " << packet_->pts
                //                             << " sample rate: " << audioCodecCtx_->sample_rate
                //                             << " channels: " << audioCodecCtx_->channels
                //                             << " audioFormat: " << audioCodecCtx_->sample_fmt
                //                             << " sameples: " << audioFrame_->nb_samples;
                av_frame_unref(audioFrame_.get());
                packet_->size -= decodedsize;
                packet_->data += decodedsize;
                if (packet_->size != 0) {
                    Log("######## decode audio not none");
                }
            }while(packet_->size > 0);
            av_packet_unref(packet_.get());
            av_free_packet(packet_.get());
        }
        else {
            if (packet_->stream_index != videoStreamId_) {
                av_packet_unref(packet_.get());
                av_free_packet(packet_.get());
                return 0;
            }
            int64_t pre = NanoTime();
            // Decode video packet
            int decodedsize = avcodec_decode_video2(videoCodecCtx_, videoFrame_.get(), &gotframe, packet_.get());
            // Get a decoded video frame
            if (gotframe) {
                got_nums_++;
                int64_t current_pos = av_rescale_q(videoFrame_->pts,   videoStream_->time_base, AV_TIME_BASE_Q);
                videoFrame_->pts = current_pos;
                tm = current_pos/1000*1000;//videoFrame_->pts;
                //Log("stream tb %d %d time %lld", videoStream_->time_base.num, videoStream_->time_base.den, current_pos);
                if (got_nums_ % 1 == 0) {
                    int64_t post = NanoTime();
                    Log("get decode size: %d, tm:%lldms used %d, frame pts:%d, pkt_pts:%d, coded num:%d, disp num:%d width %d height %d channels %d stride %d %d %d format %d del %d",
                        decodedsize, tm/1000, (post - pre) /1000/ 1000, videoFrame_->pts, videoFrame_->pts,
                        videoFrame_->coded_picture_number, videoFrame_->display_picture_number, videoFrame_->width,
                        videoFrame_->height,
                        videoFrame_->channels, videoFrame_->linesize[0], videoFrame_->linesize[1], videoFrame_->linesize[2],
                        videoFrame_->format,videoFrame_->linesize[1] - videoFrame_->linesize[0]
                        );
                }
                int lumaSize = videoFrame_->width * videoFrame_->height;
                int cromaSize = videoFrame_->width/2 * videoFrame_->height/2;


                AVFrame dst;

                memset(&dst, 0, sizeof(dst));
                //av_image_alloc
                //av_frame_get_buffer
                //av_image_fill_arrays
                int w = videoFrame_->width, h = videoFrame_->height;
                if(data) {
                      //FUNCTION_LOG;
                    if (0) {
                        dst.data[0] = (uint8_t *)(*data);
                        AVPixelFormat dst_pixfmt = AV_PIX_FMT_NV12;//AV_PIX_FMT_RGB24;//AV_PIX_FMT_NV12;
                        AVPixelFormat src_pixfmt = (AVPixelFormat)videoFrame_->format;
                        avpicture_fill( (AVPicture *)&dst, dst.data[0], dst_pixfmt, w, h);
                        struct SwsContext *convert_ctx=NULL;
                        convert_ctx = sws_getContext(w, h, src_pixfmt, w, h, dst_pixfmt,
                                                     SWS_FAST_BILINEAR, NULL, NULL, NULL);
                        sws_scale(convert_ctx, videoFrame_->data, videoFrame_->linesize, 0, h,
                                  dst.data, dst.linesize);
                        sws_freeContext(convert_ctx);

                    } else {

                        libyuv::I420ToNV12(videoFrame_->data[0], videoFrame_->linesize[0],
                                           videoFrame_->data[1], videoFrame_->linesize[1],
                                           videoFrame_->data[2], videoFrame_->linesize[2],
                                           *data, videoFrame_->width,
                                           *data+ (videoFrame_->width*videoFrame_->height),videoFrame_->width,
                                           videoFrame_->width, videoFrame_->height);
                    }


                }
                if (result) {
                    av_frame_ref(result, videoFrame_.get());
                }

//                avpicture_free((AVPicture *)&dst);
                //memcpy(*data, videoFrame_->data[0], lumaSize);
                av_frame_unref(videoFrame_.get());
            }
            //assert(decodedsize == packet_->size);
            av_packet_unref(packet_.get());
            av_free_packet(packet_.get());
            return 0;
        }
    return 0;
}

void Demuxer::SetLoop(bool loop) {
    loop_ = loop;
}