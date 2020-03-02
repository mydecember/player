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

class FuncUsed {
public:
    FuncUsed(const char* fun):mName(fun) {
        tm = MilliTime();
        //Log("enter %s time %ld", mName, tm);
    }
    ~FuncUsed() {
        long le = MilliTime();
        Log("leave %s time %lld used time %ld", mName, le, (le-tm));
    }
private:
    long tm;
    const char* mName;
};

#define SCOPTED_LOG_IMP(X) FuncUsed log(X)
#define SCOPTED_LOG SCOPTED_LOG_IMP(__func__)
#define FUNCTION_LOG SCOPTED_LOG
template<typename T>
struct Average {
    T total;
    int cnt;
    Average() : total(0), cnt(0) {}
    void add(T v) {
        total += v;
        cnt ++;
    }
    T avg() {
        return cnt == 0 ? 0 : total/cnt;
    }
};

#endif //TESTCODEC_UTILS_H
