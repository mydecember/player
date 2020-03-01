//
// Created by zz on 20-2-28.
//

#include <render/texture_render.h>
#include <unistd.h>
#include "Player.h"

Player::Player():running_(false) {

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
    demuxer.Open("/sdcard/voip-data/c.mp4");
    int got;
    int W = 1920;
    int H = 1080;
    int len = W*H*3;
    uint8_t  *data  = (uint8_t  *)malloc(len ) ;
    int ret = 0;
    running_ = true;
    int64_t pre = MilliTime();
    int64_t tm;
    Log("1111111");

    Log("222222222");
    while( ret == 0 && running_) {
        long want = 33000;
        pre = MilliTime();
        while((ret = demuxer.GetFrame((uint8_t**)&data, len , got, tm)) == 0) {
            if (got) {
                display_->Display(data, len, W, H);
                break;
            }
        }
        long used = MilliTime() - pre;
        //Log("used ms %d", used);
        if (want - used*1000 >0)
        usleep(want - used*1000);
    }
    Log("to close ");
    demuxer.Close();
    int64_t post = MilliTime();
    free(data);

    Log("decode used time %lld", (post - pre));
}


