//
// Created by zz on 19-11-14.
//

#include "Demuxer.h"

#include<mutex>
#include<thread>
#include "utils.h"


//using namespace media_ffmpeg;
using namespace std;
once_flag g_init_flag;
#include <android/log.h>
static char * LOG_TAG = "ffmpeg-tag";
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
static void log_callback_test2(void *ptr, int level, const char *fmt, va_list vl)
{
    va_list vl2;
    char *line = (char*)malloc(128 * sizeof(char));
    static int print_prefix = 1;
    va_copy(vl2, vl);
    av_log_format_line(ptr, level, fmt, vl2, line, 128, &print_prefix);
    va_end(vl2);
    line[127] = '\0';
    LOGE("%s", line);
    free(line);
}
void Register() {
    av_register_all();
    avcodec_register_all();
//    avcodec_register();
//    av_log_set_level(AV_LOG_INFO);
//    av_log_set_callback(log_callback_test2);
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
    videoGetFrameNums_ =0;

}
Demuxer::~Demuxer() {

}
int Demuxer::Open(std::string inputFile, int streamType) {
    streamType_ = streamType;
    Log("to open %s", inputFile.c_str());
    int ret = avformat_open_input(&av_fmt_ctx, inputFile.c_str(), NULL, NULL);
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
        auto ctx = av_fmt_ctx->streams[i]->codecpar;
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
        videoStream_ = av_fmt_ctx->streams[videoStreamId_];//
        videoStream_->discard = AVDISCARD_DEFAULT;
        AVCodec* videoCodec_ = nullptr;

        //videoCodec_ = avcodec_find_decoder(videoStream_->codecpar->codec_id);
        videoCodec_ = avcodec_find_decoder_by_name("h264_mediacodec");
        videoCodecCtx_ = avcodec_alloc_context3(videoCodec_);
        if (videoCodec_ == nullptr) {
            Log("NNNNNNNNNNNNN");
            return -1;
        }
        avcodec_parameters_to_context(videoCodecCtx_, videoStream_->codecpar);


        Log("find video stream width:%d height:%d", videoCodecCtx_->width, videoCodecCtx_->height);

        //get frame rate for decoder
        videoCodecCtx_->framerate = videoStream_->avg_frame_rate;
        if (videoCodec_ == nullptr) {
            videoCodecCtx_ = nullptr;
            Log("Cannot find video codec");
            return -4;
        }
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
       // audioCodecCtx_ = audioStream_->codec;

        audioCodec_ = avcodec_find_decoder(audioStream_->codecpar->codec_id);
        audioCodecCtx_ = avcodec_alloc_context3(audioCodec_);
        avcodec_parameters_to_context(audioCodecCtx_, audioStream_->codecpar);

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
    Log("open file ok video id %d audio id %d", videoStreamId_, audioStreamId_);
    return ret;
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

int av_decode_frame(AVCodecContext *dec_ctx, AVPacket *packet, bool *new_packet, AVFrame *frame)
{
    int ret = AVERROR(EAGAIN);

    while (1)
    {
        // 2. 从解码器接收frame
        if (dec_ctx->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            // 2.1 一个视频packet含一个视频frame
            //     解码器缓存一定数量的packet后，才有解码后的frame输出
            //     frame输出顺序是按pts的顺序，如IBBPBBP
            //     frame->pkt_pos变量是此frame对应的packet在视频文件中的偏移地址，值同pkt.pos
            ret = avcodec_receive_frame(dec_ctx, frame);
            if (ret >= 0)
            {
                if (frame->pts == AV_NOPTS_VALUE)
                {
                    frame->pts = frame->best_effort_timestamp;
                }
            }
        }
        else if (dec_ctx->codec_type ==  AVMEDIA_TYPE_AUDIO)
        {
            // 2.2 一个音频packet含一至多个音频frame，每次avcodec_receive_frame()返回一个frame，此函数返回。
            //     下次进来此函数，继续获取一个frame，直到avcodec_receive_frame()返回AVERROR(EAGAIN)，
            //     表示解码器需要填入新的音频packet
            ret = avcodec_receive_frame(dec_ctx, frame);
            if (ret >= 0)
            {
                if (frame->pts == AV_NOPTS_VALUE)
                {
                    frame->pts = frame->best_effort_timestamp;
                }
            }
        }

        if (ret >= 0)                   // 成功解码得到一个视频帧或一个音频帧，则返回
        {
            return ret;
        }
        else if (ret == AVERROR_EOF)    // 解码器已冲洗，解码中所有帧已取出
        {
            avcodec_flush_buffers(dec_ctx);
            return ret;
        }
        else if (ret == AVERROR(EAGAIN))// 解码器需要喂数据
        {
            if (!(*new_packet))         // 本函数中已向解码器喂过数据，因此需要从文件读取新数据
            {
                //av_log(NULL, AV_LOG_INFO, "decoder need more packet\n");
                return ret;
            }
        }
        else                            // 错误
        {
            Log("decoder error %d\n", ret);
            return ret;
        }

        /*
        if (packet == NULL || (packet->data == NULL && packet->size == 0))
        {
            // 复位解码器内部状态/刷新内部缓冲区。当seek操作或切换流时应调用此函数。
            avcodec_flush_buffers(dec_ctx);
        }
        */

        // 1. 将packet发送给解码器
        //    发送packet的顺序是按dts递增的顺序，如IPBBPBB
        //    pkt.pos变量可以标识当前packet在视频文件中的地址偏移
        //    发送第一个 flush packet 会返回成功，后续的 flush packet 会返回AVERROR_EOF
        ret = avcodec_send_packet(dec_ctx, packet);


        if (ret != 0)
        {
            Log("avcodec_send_packet() error, return %d\n", ret);
            return ret;
        } else {
            *new_packet = false;
        }
    }
}

static int sendPackets = 0;
bool newPacket = false;
int Demuxer::GetFrame(uint8_t** data, int& len , int &gotframe) {
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
//        avcodec_decode_video2(videoCodecCtx_, videoFrame_.get(), &gotframe, packet_.get());
//        int avcodec_send_packet(AVCodecContext *avctx, const AVPacket *avpkt);
        avcodec_send_packet(videoCodecCtx_, packet_.get());
        int ret = avcodec_receive_frame(videoCodecCtx_, videoFrame_.get());
        if (ret < 0 && ret != AVERROR(EAGAIN))
            return 0;
        if (ret >= 0)
            gotframe = 1;

        if (gotframe) {
            Log(" zfq get video frame1  used ");
            av_frame_unref(videoFrame_.get());
            //if (videoData_.size() != videoFrame_->linesize)
            //*data = videoFrame_->pkt_pts;
            return 0;
        } else {
            videoDecodeEof_ = true;
        }
    }
//    if (!audioDecodeEof_ && audioCodecCtx_ && audioStreamEof_) {
//        packet_->data = NULL;
//        packet_->size = 0;
//        avcodec_decode_audio4(audioCodecCtx_, audioFrame_.get(), &gotframe, packet_.get());
//        if (gotframe) {
//            Log("zfq get audio frame eof");
//
//            av_frame_unref(audioFrame_.get());
//            return 0;
//        } else {
//            audioDecodeEof_ = true;
//        }
//    }

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

    int ret  = 0;
    {
        bool pre = newPacket;
        int result = av_decode_frame(videoCodecCtx_, packet_.get(), &newPacket, videoFrame_.get());

        if (result == AVERROR_EOF) {
            return -1;
        }
        if (result >= 0) {
            int64_t post = MilliTime();
            videoGetFrameNums_++;
            Log("get send %d decode num %d  used %d, frame pts:%lld, pkt_pts:%lld, coded num:%d, disp num:%d, format:%d %d linesize %d",
                sendPackets, videoGetFrameNums_, (int)(post ), videoFrame_->pts, videoFrame_->pts,
                videoFrame_->coded_picture_number, videoFrame_->display_picture_number, videoFrame_->format, AV_PIX_FMT_NV12, videoFrame_->linesize[0]);
            return 0;
        }
        if (AVERROR(EAGAIN) == result && pre == true && newPacket == false) {

        }

    }
    if (!newPacket) {
        ret = av_read_frame(av_fmt_ctx, packet_.get());
        newPacket = true;
        sendPackets++;
        if (ret < 0) {
            if(ret == AVERROR_EOF) {
                Log("av_read_frame AVERROR_EOF");
                audioStreamEof_ = true;
                videoStreamEof_ = true;
                avcodec_send_packet(videoCodecCtx_, NULL);
                return -1;
            }
            return -1;
        }
        return 0;
    } else {
        return 0;
        if (packet_->stream_index != videoStreamId_) {
            av_packet_unref(packet_.get());
            //av_free_packet(packet_.get());
            return 0;
        }
        int64_t pre = MilliTime();
        // Decode video packet
       // int decodedsize = avcodec_decode_video2(videoCodecCtx_, videoFrame_.get(), &gotframe, packet_.get());
        sendPackets++;

        int result = av_decode_frame(videoCodecCtx_, packet_.get(), &newPacket, videoFrame_.get());

        if (result == AVERROR_EOF) {
            return -1;
        }
        if (result >= 0) {
            int64_t post = MilliTime();
            videoGetFrameNums_++;
            Log("get send %d decode num %d  used %d, frame pts:%lld, pkt_pts:%lld, coded num:%d, disp num:%d, format:%d %d",
               sendPackets, videoGetFrameNums_, (int)(post - pre), videoFrame_->pts, videoFrame_->pts,
               videoFrame_->coded_picture_number, videoFrame_->display_picture_number, videoFrame_->format, AV_PIX_FMT_NV12);
        }
        return 0;

        int ret = avcodec_send_packet(videoCodecCtx_, packet_.get());
        Log("send packet ret %d", ret);
        if (ret < 0) {
            Log("failed to send packet %d", ret);
        }

        while(ret>=0) {
            ret = avcodec_receive_frame(videoCodecCtx_, videoFrame_.get());
            if (ret < 0 && ret == AVERROR(EAGAIN))
                break;
            if (ret >= 0)
                gotframe = 1;

            // Get a decoded video frame
            if (gotframe) {
                videoGetFrameNums_++;
                int64_t post = MilliTime();
                AVPixelFormat;
                Log("get send %d decode num %d  used %d, frame pts:%lld, pkt_pts:%lld, coded num:%d, disp num:%d, format:%d %d",
                    sendPackets, videoGetFrameNums_, (int)(post - pre), videoFrame_->pts, videoFrame_->pts,
                    videoFrame_->coded_picture_number, videoFrame_->display_picture_number, videoFrame_->format, AV_PIX_FMT_NV12);
                //   av_frame_unref(videoFrame_.get());
            }
        }
        ret = avcodec_receive_frame(videoCodecCtx_, videoFrame_.get());
        if (ret < 0 && ret != AVERROR(EAGAIN))
            return 0;
        if (ret >= 0)
            gotframe = 1;

        // Get a decoded video frame
        if (gotframe) {
            videoGetFrameNums_++;
            int64_t post = MilliTime();
            AVPixelFormat;
            Log("get decode sendPackets %d get num %d  used %d, frame pts:%lld, pkt_pts:%lld, coded num:%d, disp num:%d, format:%d %d",
                sendPackets, videoGetFrameNums_, (int)(post - pre), videoFrame_->pts, videoFrame_->pts,
                videoFrame_->coded_picture_number, videoFrame_->display_picture_number, videoFrame_->format, AV_PIX_FMT_NV12);
         //   av_frame_unref(videoFrame_.get());
        }

        av_packet_unref(packet_.get());
//        av_free_packet(packet_.get());
        return 0;
    }
    return 0;
}

void Demuxer::SetLoop(bool loop) {
    loop_ = loop;
}