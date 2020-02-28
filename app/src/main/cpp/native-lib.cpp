#include <jni.h>
#include <string>
#include "Demuxer.h"
#include "thread"
#include "syslog.h"
#include <android/log.h>
#include "utils.h"
extern "C" {
#include "libavcodec/avcodec.h"
#include <libavformat/avformat.h>
#include "libavcodec/jni.h"
}


void Run() {
    Demuxer demuxer;
    demuxer.Open("/sdcard/voip-data/dou.mp4");
    int len ;
    int got;
    uint8_t*  data;
    int64_t pre = MilliTime();
    while(demuxer.GetFrame(&data, len , got) == 0) {

    }
    Log("to close ");
    demuxer.Close();
    int64_t post = MilliTime();

    Log("decode used time %lld", (post - pre));
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
    //new std::thread(Run);
}
extern "C"
JNIEXPORT
jint JNI_OnLoad(JavaVM *vm, void *res) {
    av_jni_set_java_vm(vm, 0);
    // 返回jni版本
    return JNI_VERSION_1_4;


}
