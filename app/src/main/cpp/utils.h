//
// Created by zz on 19-11-15.
//

#ifndef TESTCODEC_UTILS_H
#define TESTCODEC_UTILS_H
#include "thread"
#include "syslog.h"
#include <android/log.h>
void Log(const char * fmt, ...);
void Logs(const char * str);
int64_t MilliTime();
int64_t NanoTime() ;

int64_t MacroTime() ;

#endif //TESTCODEC_UTILS_H
