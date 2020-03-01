#include <jni.h>
#include <string>
#include "Demuxer.h"
#include "thread"
#include "syslog.h"
#include <android/log.h>
#include <unistd.h>
#include <media/NdkMediaCodec.h>
#include "base/utils.h"
#include "Player.h"
#include "HWVideoEncoder.h"
#include <android/native_window_jni.h>
#include <render/egl_base.h>
#include <render/texture_render.h>

extern "C" {
#include "libavcodec/avcodec.h"
#include <libavformat/avformat.h>
#include "libavcodec/jni.h"
}

void Run() {
    Demuxer demuxer;
    demuxer.Open("/sdcard/voip-data/dou.mp4");
    int got;
    int W = 1920;
    int H = 1080;
    int len = W*H*3/2;
    uint8_t  *data  = (uint8_t  *)malloc(W*H*3/2 ) ;
    int64_t pre = MilliTime();
    HWVideoEncoder encoder;
    int64_t tm;
    encoder.start("/sdcard/tt.mp4", W, H);

    int ret = 0;
    int64_t t = MilliTime();
    int64_t wantTime = 33;
    int64_t t1 = MilliTime();
    while( ret == 0) {
        ret = demuxer.GetFrame((uint8_t**)&data, len , got, tm, NULL);
        if (got) {
            encoder.encodeFrame(tm, data, len);
            int64_t t2 = MilliTime();

            Log("used time %lld",(t2-t1) );
            if (t2 - t < wantTime) {
                //::usleep((wantTime - (t2 - t))*1000 );
            }
            wantTime += 33;
            t1 = t2;

        }
    }
    Log("to close ");
    demuxer.Close();
    int64_t post = MilliTime();
    free(data);

    Log("decode used time %lld", (post - pre));
    encoder.stop();
    encoder.release();
}

std::thread* g_thread ;//= new std::thread(Run);

extern "C" JNIEXPORT jstring JNICALL
Java_com_xm_testcodec_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}

extern "C" JNIEXPORT void JNICALL
Java_com_xm_testcodec_MainActivity_startJNI(
        JNIEnv *env,
        jobject /* this */) {
    Log("to start");
    g_thread = new std::thread(Run);
    g_thread->detach();

//    g_thread = new std::thread(Run);
//    g_thread->detach();
}

extern "C"
JNIEXPORT
jint JNI_OnLoad(JavaVM *vm, void *res) {
    av_jni_set_java_vm(vm, 0);
    // 返回jni版本
    return JNI_VERSION_1_4;
}

extern "C" JNIEXPORT jlong JNICALL
Java_com_xm_player_MiPlayer__1nativeSetup(
        JNIEnv *env,
        jobject /* this */,
        jobject weak_this) {
    Log("MiPlayer created");
    Player *mp = new Player();
    if (!mp)
        return 0;
    return (*(long*)&mp);
}

extern "C" JNIEXPORT void JNICALL
Java_com_xm_player_MiPlayer__1setVideoSurface(
        JNIEnv *env,
        jobject /* this */,
        jlong p,
        jobject surface
        ) {
    Log("set surface");
    Player* player = (Player*)p;
    ANativeWindow *native_window = NULL;
    if (surface) {
        native_window = ANativeWindow_fromSurface(env, surface);
        if (!native_window) {
            Log("%s: ANativeWindow_fromSurface: failed\n", __func__);
            // do not return fail here;
        }
        player->SetSurface(native_window);
    } else {
        player->SetSurface(nullptr);
    }
    //env->IsSameObject(env, surface, prev_surface)

    //SDL_VoutAndroid_SetNativeWindow(vout, native_window);
//    if (native_window)
//        ANativeWindow_release(native_window);
}

extern "C" JNIEXPORT void JNICALL
        Java_com_xm_player_MiPlayer__1start(
                JNIEnv *env,
                jobject /* this */,
                jlong p
                ) {
    Player* player = (Player*)p;
    player->Start();
}

extern "C" JNIEXPORT void JNICALL
Java_com_xm_player_MiPlayer__1scale(
        JNIEnv *env,
        jobject /* this */,
        jlong p,
        jfloat value
) {
    Player* player = (Player*)p;
    player->Scale(value);
}

extern "C" JNIEXPORT void JNICALL
Java_com_xm_player_MiPlayer__1drag(
        JNIEnv *env,
        jobject /* this */,
        jlong p,
        jfloat x,
        jfloat y,
        jfloat xe,
        jfloat ye
) {
    Player* player = (Player*)p;
    Log("111 %f, %f, %f, %f", x, y, xe, ye);
    player->Drag(x, y, xe, ye);
}