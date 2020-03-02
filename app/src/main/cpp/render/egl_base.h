
#ifndef __EGL_BASE__
#define __EGL_BASE__

#include <jni.h>
#include <android/native_window.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

class EglBase {
public:
    EglBase();
    int create();
    int createSurface(ANativeWindow *window);
    int createPbufferSurface(int width, int height);
    void swapBuffers();
    int makeCurrent();
    void unMakeCurrent();
    void release();
    int getSurfaceWidth();
    int getSurfaceHeight();

private:
    EGLDisplay _egl_display;
    EGLConfig  _egl_config;
    EGLContext _egl_context;
    EGLSurface _egl_surface;
};

#endif
