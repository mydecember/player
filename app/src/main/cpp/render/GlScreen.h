//
// Created by zfq on 2020/2/29.
//

#ifndef TESTCODEC_GLSCREEN_H
#define TESTCODEC_GLSCREEN_H

#include "texture_render.h"
#include "egl_base.h"
#include <render/egl_base.h>
#include <GLES2/gl2ext.h>
#include "memory"
#include <mutex>
#include "../Demuxer.h"
#include <android/native_window.h>
#include "tex_transform_util.h"
struct Pointf {
    float x;
    float y;
    Pointf(float x=0.0, float y=0.0):x(x),y(y){}
    bool operator != (const Pointf& p) {
        if (x != p.x || y != p.y) {
            return true;
        }
        return false;
    }
};
class GlScreen {
public:
    GlScreen();
    ~GlScreen();
    void SetSurface(ANativeWindow* surface);
    void Display(uint8_t* data, int len, int w , int h);
    void Scale(float scale);
    void SetDrag(float x, float y, float xe, float ye);
private:

private:
    std::unique_ptr<TextureRender> textureRender_;
    Texture txt_;
    std::unique_ptr<EglBase> eglBase_;
    std::mutex mutex_;
    ANativeWindow* surface_;
    GLfloat m_mvp[16];
    float scale_;
    Pointf dragStartPoint_;
    Pointf dragStartCenter_;
    Pointf detPoint_;
    Pointf center_;

};


#endif //TESTCODEC_GLSCREEN_H
