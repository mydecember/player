//
// Created by zfq on 2020/3/14.
//
#ifndef TESTCODEC_MUXER_H
#define TESTCODEC_MUXER_H

#ifdef __cplusplus
extern "C" {
#endif
#include <libavformat/avformat.h>
#include <libavutil/frame.h>
#include "libavcodec/avcodec.h"
#include <libswscale/swscale.h>
#include <ffmpeg/libavutil/avstring.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#ifdef __cplusplus
}
#endif

class Muxer {
public:
    Muxer();
    ~Muxer();
    bool InitEncoder();
    void Encode(AVFrame* frame);

private:
    AVCodec *codec_;
    AVCodecContext *codecContext_= NULL;
    AVPacket pkt_;
};


#endif //TESTCODEC_MUXER_H
