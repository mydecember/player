//
// Created by zz on 20-2-28.
//

#include <render/texture_render.h>
#include <unistd.h>
#include "Player.h"
#include "libyuv/include/libyuv.h"
Player::Player():running_(false) {
    libyuv::FixedDiv(10, 5);
}
Player::~Player() {

}
void Player::Open(std::string url) {

}
void Player::SetSurface(ANativeWindow* window) {
    window_ = window;
    if (!display_) {
        display_.reset(new GlScreen());
    }
    display_->SetSurface(window_);
}
void Player::Start() {
    Log("player start");
    thread_.reset(new std::thread(&Player::Run, this));
    thread_->detach();
}
void Player::Pause() {

}
void Player::Close() {

}
void Player::Scale(float scale) {
    display_->Scale(scale);
}

void Player::Drag(float x, float y, float xe, float ye) {
    Log("222 %f, %f, %f, %f", x, y, xe, ye);
    display_->SetDrag(x, y, xe, ye);
}

void Player::Run() {
    Log("player run");
    Demuxer demuxer;
    //demuxer.Open("/sdcard/voip-data/dou.mp4");
    //demuxer.Open("rtmp://58.200.131.2:1935/livetv/hunantv");
    if (demuxer.Open("http://ivi.bupt.edu.cn/hls/cctv1hd.m3u8", 3)) {
        Log("open error");
        return ;
    }
    int got;
    int W = demuxer.GetWidth();
    int H = demuxer.GetHeight();
    int len = W*H*3;
    uint8_t  *data  = (uint8_t  *)malloc(len ) ;
    int ret = 0;
    running_ = true;
    int64_t pre = MilliTime();
    int64_t tm;
    Log("1111111");

    AVFrame * frame = av_frame_alloc();

    Log("222222222");
    while( ret == 0 && running_) {
        long want = 33000;
        pre = MilliTime();
        while((ret = demuxer.GetFrame(NULL, len , got, tm, frame)) == 0) {
            if (got) {
                libyuv::I420ToRGB24(frame->data[0], frame->linesize[0],
                        frame->data[2], frame->linesize[2],
                        frame->data[1], frame->linesize[1],
                        data,frame->width*3, frame->width, frame->height);

                display_->Display(data, len, W, H);
                av_frame_unref(frame);
                break;
            }
        }
        long used = MilliTime() - pre;
        //Log("used ms %d pts %lld", used, tm);
        if (want - used*1000 >0)
        usleep(want - used*1000 );
    }
    Log("to close ");
    demuxer.Close();
    int64_t post = MilliTime();
    free(data);
    av_frame_free(&frame);
    Log("decode used time %lld", (post - pre));
}


