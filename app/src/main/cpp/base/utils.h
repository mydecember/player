//
// Created by zz on 19-11-15.
//

#ifndef TESTCODEC_UTILS_H
#define TESTCODEC_UTILS_H
#include "thread"
#include "syslog.h"
#include <android/log.h>
#include <cstdint>
#include <string>
#include <mutex>
#include <memory>
#include <atomic>
#include <vector>
#include <deque>
#include <map>
#include <set>
#include <sys/stat.h>
#include <dirent.h>
#include "string"
#include <sstream>

using namespace std;
#define CHECK_NULL_CODE(obj, code)         if (!obj)  return code;
#define CHECK_NULL(obj)                    CHECK_NULL_CODE(obj, -1)
#define CHECK_RESULT(result)               if (result < 0) return result;
#define CHECK_NULL_GOTO(obj, lebal)        if (!obj)  goto lebal;
#define CHECK_RESULT_GOTO(result, lebal)   if (result < 0) goto lebal;
void Log(const char * fmt, ...);
void Logs(const char * str);
int64_t ToInt64(const string& v);
int32_t ToInt32(const string& v);
float ToFloat(const string& v);

string ToString(unsigned int v);

string ToString(int v);

string ToString(int64_t v);

string ToString(float v);

string ToString(double v);

bool StartsWith(const string& s, const string& prefix);
bool EndsWith(const string& s, const string& suffix);
string ReplaceExtension(const string& name, const string& ext);

string DirName(const string& path);
const char * FileName(const char * path);
bool IsDir(const string& path);
bool IsFile(const string& path);
bool Mkdirs(const string& path);
bool ListDir(const string& dir, vector<string>& names);
bool ReadBinaryFile(const string& file, vector<uint8_t>& data);
bool WriteBinaryFile(const string& file, vector<uint8_t>& data);

string ToLower(const string& s);
string GetExt(const string& name);

string Format(const char * fmt, ...);
string FormatNow(const char * fmt);
string FormatNowSimple();
double Time();
int64_t NanoTime();
int64_t MacroTime();
int64_t MacroCPUTime();
int64_t MilliTime();


std::string Base64Encode(unsigned char const* , unsigned int len);
std::string Base64Decode(std::string const& s);

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
#define LOG \
    Logging(__FILE__, __LINE__).stream()

class Logging {
public:
    Logging(std::string file, int line) {
        file_ = file;
        line_ = line;
        print_stream_<<"["<<file_<<":"<<line_<<"]";
    }

    ~Logging() {
        std::unique_lock<mutex> lk(mutex_);
        Log("%s", print_stream_.str().c_str());
    }
//    static Logging* GetInstance(){
//        static Logging log = Logging();
//        return &log;
//    }

    std::ostream& stream() {
        return print_stream_;
    }
private:
    std::ostringstream print_stream_;
    std::mutex mutex_;
    std::string file_;
    std::string line_;

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

#define DECLARE_DUMP_FILE(file_name, variable_name) \
    static FILE* variable_name = NULL; \
    if (!(variable_name)) \
    { \
       variable_name = fopen(file_name, "wb"); \
    }

#define DUMP_UTIL_FILE_WRITE(variable_name,  data, len) \
    if (variable_name) { \
        fwrite(data, len, sizeof(char), variable_name); \
    }

#endif //TESTCODEC_UTILS_H
