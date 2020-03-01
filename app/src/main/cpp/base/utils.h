//
// Created by zz on 19-11-15.
//

#ifndef TESTCODEC_UTILS_H
#define TESTCODEC_UTILS_H
#include "thread"
#include "syslog.h"
#include <android/log.h>
#include <cstdint>
#define CHECK_NULL_CODE(obj, code)         if (!obj)  return code;
#define CHECK_NULL(obj)                    CHECK_NULL_CODE(obj, -1)
#define CHECK_RESULT(result)               if (result < 0) return result;
#define CHECK_NULL_GOTO(obj, lebal)        if (!obj)  goto lebal;
#define CHECK_RESULT_GOTO(result, lebal)   if (result < 0) goto lebal;
void Log(const char * fmt, ...);
void Logs(const char * str);
int64_t MilliTime();
int64_t NanoTime() ;

int64_t MacroTime() ;

#endif //TESTCODEC_UTILS_H
