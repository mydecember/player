//
// Created by zz on 19-11-15.
//
#include "utils.h"
void Log(const char * fmt, ...) {
    va_list arglist;
    va_start(arglist, fmt);
    __android_log_vprint(ANDROID_LOG_INFO, "codec", fmt, arglist);
    va_end(arglist);
}

void Logs(const char * str) {
    __android_log_write(ANDROID_LOG_INFO, "codec", str);
}
int64_t MilliTime() {
    timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
    return spec.tv_sec * (int64_t)1000L + spec.tv_nsec/1000000;
}
int64_t NanoTime() {
    timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
    return spec.tv_sec * (int64_t)1000000000L + spec.tv_nsec;
}

int64_t MacroTime() {
    timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
    return spec.tv_sec * (int64_t)1000000L + spec.tv_nsec/1000;
}