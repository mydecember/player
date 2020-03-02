//
// Created by zfq on 2020/2/29.
//

#include "GlScreen.h"
#include "base/utils.h"
GlScreen::GlScreen():scale_(1.0),center_(0.5,0.5) {
screenW_ =0;
screenH_ = 0;
}
GlScreen::~GlScreen() {
    //SetSurface(nullptr);
}
void GlScreen::SetSurface(ANativeWindow* surface) {
    std::lock_guard<std::mutex> lg(mutex_);
    surface_ = surface;
    if (surface == nullptr) {
        if(eglBase_)
            eglBase_->release();
        if (textureRender_)
            textureRender_->release();
        textureRender_.reset();
        txt_.release();
        eglBase_.reset();
        return;
    }
}

void GlScreen::Scale(float scale) {
    std::lock_guard<std::mutex> lg(mutex_);
    //textureRender_->setScale(scale);
    if (scale != 0)
    scale_ *= scale;
}
void GlScreen::SetDrag(float x, float y, float xe, float ye) {
    std::lock_guard<std::mutex> lg(mutex_);
    Log("== %f , %f ,%f,%f", x, y, xe, ye);
    Pointf start(x,y);
    if (start != dragStartPoint_) {
        dragStartPoint_ = start;
        dragStartCenter_ = detPoint_;
    }
//    Pointf v(x-xe, y-ye);
    detPoint_.x = dragStartCenter_.x + (xe-x)/screenW_;
    detPoint_.y = dragStartCenter_.y + (y - ye)/screenH_;
    Log("-->%f %f %f %f", (xe-x) , (ye-y), detPoint_.x, detPoint_.y);

}

void GlScreen::Display(uint8_t* data, int len, int w , int h) {
    std::lock_guard<std::mutex> lg(mutex_);
    if (!eglBase_ && surface_) {
        eglBase_.reset(new EglBase());
        eglBase_->create();

        eglBase_->createSurface(surface_);
        eglBase_->makeCurrent();
        textureRender_.reset(new TextureRender(GL_TEXTURE_2D));
        textureRender_->init();
        screenW_ = eglBase_->getSurfaceWidth();
        screenH_ = eglBase_->getSurfaceHeight();
        textureRender_->setScreenFrame(screenW_, screenH_, w, h);
        Log("SSSSSSSSSSSSSSSSSSSs w %d %d", screenW_, screenH_);
        eglBase_->unMakeCurrent();
    }

    if (!eglBase_ || !textureRender_) {
        return;
    }
    mtxLoadIdentity(m_mvp);
    GLfloat scaleMatrix[16];
    mtxLoadIdentity(scaleMatrix);
    mtxLoadScale(scaleMatrix, scale_, scale_, 1.0);

    GLfloat translateMatrix[16];
    mtxLoadIdentity(translateMatrix);

   mtxTranslateMatrix(translateMatrix, detPoint_.x*scale_, detPoint_.y*scale_, 0.0);

    mtxMultiply(m_mvp, translateMatrix, scaleMatrix);
    eglBase_->makeCurrent();
    txt_.set(w, h, 3, data);
    textureRender_->draw(txt_.id, m_mvp);
    eglBase_->swapBuffers();
    eglBase_->unMakeCurrent();
}