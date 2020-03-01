//
// Created by zz on 19-11-14.
//

#ifndef DEXCAM_DEMUXER_H
#define DEXCAM_DEMUXER_H
//#include "ffmpeg/libavcodec"

#ifdef __cplusplus
extern "C" {
#endif
#include <libavformat/avformat.h>
#include <libavutil/frame.h>
#include "libavcodec/avcodec.h"
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#ifdef __cplusplus
}
#endif

#include "string"
#include "memory"
#include "vector"
//using namespace media_ffmpeg;

//enum FrameType {
//    FRAME_AUDIO,
//    FRAME_VIDEO
//};
//
//struct Frame {
//    int64_t ts_; // ms
//    int widht_;
//    int height_;
//    int stride_;
//    int color_format_;
//    FrameType frame_type_;
//    std::vector<uint8_t> buf;
//    HWVideoFrame():ts_(0), widht_(0), height_(0), stride_(0), color_format_(0){}
//};

class Demuxer {
public:
    Demuxer();
    ~Demuxer();
    // streamType 1 audio only 2 video only 3 audio and video
    int Open(std::string inputFile, int streamType = 2);
    void SetLoop(bool loop);
    //
    int GetFrame(uint8_t** data, int& len , int &got, int64_t& tm);
    int Close();
private:
    void FlushDecoderBuffers();
    void FrameDataToVector(AVFrame* frame, bool isAudio);
private:
    int streamType_;
    std::unique_ptr<AVPacket> packet_;
    std::unique_ptr<AVFrame> audioFrame_;
    std::unique_ptr<AVFrame> videoFrame_;

    int videoStreamId_;
    int audioStreamId_;
    AVFormatContext* av_fmt_ctx;
    AVStream* videoStream_;
    AVStream* audioStream_;
    AVCodecContext* videoCodecCtx_;
    AVCodecContext* audioCodecCtx_;
    bool videoStreamEof_;
    bool audioStreamEof_;
    bool videoDecodeEof_;
    bool audioDecodeEof_;
    bool demuxerEnd_;
    bool loop_;
    std::vector<uint8_t> videoData_;
    std::vector<uint8_t> audioData_;
    int got_nums_;

};




#endif //DEXCAM_DEMUXER_H
