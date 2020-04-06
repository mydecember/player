//
// Created by zz on 20-2-28.
//

#ifndef TESTCODEC_PLAYER_H
#define TESTCODEC_PLAYER_H
#include <string>
#include "Demuxer.h"
#include "thread"
#include "syslog.h"
#include <android/log.h>
#include <android/native_window.h>
#include <GLES2/gl2.h>
#include "base/utils.h"
#include "render/egl_base.h"
#include "render/texture_render.h"
#include "render/GlScreen.h"
#include "audio_device/OpenSLESPlayer.h"
class Player {
public:
    Player();
    ~Player();
    void Open(std::string url);
    void SetSurface(ANativeWindow* window );
    void Start();
    void Pause();
    void Close();
    void Scale(float scale);
    void Drag(float x, float y, float xe, float ye);
private:
    void Run();
private:
    std::string url_;
    std::atomic<bool> running_;
    std::unique_ptr<std::thread> thread_;
    std::mutex lock_;
    std::unique_ptr<GlScreen> display_;
    ANativeWindow* window_;
    std::unique_ptr<OpenSLESPlayer> audioPlayer_;
};



#endif //TESTCODEC_PLAYER_H
