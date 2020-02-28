#include <jni.h>
#include <string>
#include "Demuxer.h"
#include "thread"
#include "syslog.h"
#include <android/log.h>
#include <unistd.h>
#include "utils.h"
#include "Player.h"
#include "HWVideoEncoder.h"
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
        ret = demuxer.GetFrame((uint8_t**)&data, len , got, tm);
        if (got) {
            encoder.encodeFrame(tm, data, len);
            int64_t t2 = MilliTime();

            Log("used time %lld",(t2-t1) );
            if (t2 - t < wantTime) {
                ::usleep((wantTime - (t2 - t))*1000 );
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

extern "C" JNIEXPORT void JNICALL
Java_com_xm_testcodec_MainActivity_startJNI2(
        JNIEnv *env,
        jobject /* this */) {
    Log("to start");
    g_thread = new std::thread(Run);
    g_thread->detach();
    g_thread = new std::thread(Run);
    g_thread->detach();
}

extern "C"
JNIEXPORT
jint JNI_OnLoad(JavaVM *vm, void *res) {
    av_jni_set_java_vm(vm, 0);
    // 返回jni版本
    return JNI_VERSION_1_4;


}
