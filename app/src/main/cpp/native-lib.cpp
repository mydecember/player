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

void Run(bool toEncoder, int tag) {
    Demuxer demuxer;
    demuxer.Open("/sdcard/voip-data/dou.mp4");
    int got;
    int W = demuxer.GetWidth();
    int H = demuxer.GetHeight();
    int len = W*H*3/2;
    uint8_t  *data  = (uint8_t  *)malloc(W*H*3/2 ) ;
    int64_t pre = MilliTime();
    HWVideoEncoder encoder;
    int64_t tm;
    if (toEncoder)
    encoder.start("/sdcard/tt.mp4", W, H);

    int ret = 0;
    int64_t t = MilliTime();
    int64_t getnums = 0;
    while( ret == 0) {
        int64_t u1 = MilliTime();
        while((ret = demuxer.GetFrame((uint8_t**)&data, len , got, tm, NULL) )== 0) {
            if ((ret == 0) && got) {
                if (toEncoder)
                encoder.encodeFrame(tm*1000, data, len);
                break;
            }
        }
        getnums++;
        int64_t det = MilliTime() - t;
        int64_t pre1 = 33*getnums;

        int64_t used = MilliTime() - u1;
        //Log("used ms %d", used);

        if (pre1 - det > 0) {
            usleep((pre1 - det)*1000);
            Log(" %d sleep %lld", tag, (pre1-det));
        }
    }
    Log("to close ");
    demuxer.Close();
    int64_t post = MilliTime();
    free(data);

    Log("decode used time %lld", (post - pre));
    if (toEncoder) {
        encoder.stop();
        encoder.release();
    }

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
    g_thread = new std::thread(Run, false, 0);
    g_thread->detach();
}

extern "C" JNIEXPORT void JNICALL
Java_com_xm_testcodec_MainActivity_startJNI1(
        JNIEnv *env,
        jobject /* this */) {
    Log("to start");
    g_thread = new std::thread(Run, false, 1);
    g_thread->detach();

    g_thread = new std::thread(Run, false, 2);
    g_thread->detach();
}

extern "C" JNIEXPORT void JNICALL
Java_com_xm_testcodec_MainActivity_startJNI3(
        JNIEnv *env,
        jobject /* this */) {
    Log("to start");
    g_thread = new std::thread(Run, true, 1);
    g_thread->detach();
}

extern "C" JNIEXPORT void JNICALL
Java_com_xm_testcodec_MainActivity_startJNI2(
        JNIEnv *env,
        jobject /* this */) {
    Log("to start");
    g_thread = new std::thread(Run, true, 3);
    g_thread->detach();

    g_thread = new std::thread(Run, false, 4);
    g_thread->detach();
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
    if (!player)
        return;
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
    if (!player)
        return;
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
    if (!player)
        return;
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
    if (!player)
        return;
    Log("111 %f, %f, %f, %f", x, y, xe, ye);
    player->Drag(x, y, xe, ye);
}