//
// Created by zz on 19-11-15.
//
#include <linux/time.h>
#include <time.h>
#include <inttypes.h>
#include <libgen.h>
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


int64_t ToInt64(const string& v) {
    return (int64_t)strtoll(v.c_str(), NULL, 10);
}

int32_t ToInt32(const string& v) {
    return (int32_t)strtoll(v.c_str(), NULL, 10);
}

float ToFloat(const string& v) {
    return strtof(v.c_str(), NULL);
}

string ToString(unsigned int v) {
    char buff[32];
    snprintf(buff, 31, "%u", v);
    return buff;
}

string ToString(int v) {
    char buff[32];
    snprintf(buff, 31, "%d", v);
    return buff;
}

string ToString(int64_t v) {
    char buff[32];
    snprintf(buff, 31, "%" PRId64, v);
    return buff;
}

string ToString(float v) {
    char buff[32];
    snprintf(buff, 31, "%f", v);
    return buff;
}

string ToString(double v) {
    char buff[32];
    snprintf(buff, 31, "%lf", v);
    return buff;
}

bool StartsWith(const string& s, const string& prefix) {
    if (s.length() < prefix.length()) {
        return false;
    }
    return memcmp(s.c_str(), prefix.c_str(), prefix.length()) == 0;
}

bool EndsWith(const string& s, const string& suffix) {
    if (s.length() < suffix.length()) {
        return false;
    }
    return memcmp(s.c_str() + s.length() - suffix.length(), suffix.c_str(), suffix.length()) == 0;
}

string ReplaceExtension(const string& name, const string& ext) {
    size_t pos = name.rfind('.');
    return name.substr(0, pos) + ext;
}

string DirName(const string& path) {
    size_t pos = path.find_last_of("/");
    if (pos == string::npos) {
        return ".";
    } else if (pos == 0) {
        return "/";
    } else {
        return path.substr(0, pos);
    }
}

const char * FileName(const char * path) {
    const char * pos = strrchr(path, '/');
    if (pos == NULL) {
        return path;
    } else {
        return pos + 1;
    }
}

bool IsDir(const string& path) {
    struct stat s;
    if(stat(path.c_str(),&s) == 0) {
        if (s.st_mode & S_IFDIR) {
            return true;
        }
    }
    return false;
}

bool IsFile(const string& path) {
    struct stat s;
    if(stat(path.c_str(),&s) == 0) {
        if (s.st_mode & S_IFREG) {
            return true;
        }
    }
    return false;
}

bool Mkdirs(const string& path) {
    while (mkdir(path.c_str(), 0755) != 0) {
        if (errno == EEXIST) {
            return true;
        } else if (errno == ENOENT) {
            string path2 = path;
            if (!Mkdirs(dirname(&path2[0]))) {
                return false;
            }
        } else {
            return false;
        }
    }
    return true;
}

bool ListDir(const string& path, vector<string>& names) {
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir (path.c_str())) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            if (strcmp(".", ent->d_name) == 0 ||
                strcmp("..", ent->d_name) == 0) {
                continue;
            }
            names.emplace_back(ent->d_name);
        }
        closedir(dir);
        std::sort(names.begin(), names.end());
        return true;
    } else {
        return false;
    }
}


string GetExt(const string& name) {
    size_t pos = name.rfind('.');
    if (pos == name.npos) {
        return "";
    }
    string ext = name.substr(pos+1);
    if (ext.find('/') != ext.npos || ext.find('\\') != ext.npos) {
        return "";
    }
    return ext;
}


string Format(const char * fmt, ...) {
    char buff[2048];
    va_list arglist;
    va_start(arglist, fmt);
    vsnprintf(buff, 2047, fmt, arglist);
    va_end(arglist);
    return buff;
}

string FormatNow(const char * fmt) {
    time_t nowt;
    time(&nowt);
    struct tm now;
    localtime_r(&nowt, &now);
    char buff[256];
    strftime_l(buff, 256, fmt, &now, 0);
    return buff;
}

string FormatNowSimple() {
    return FormatNow("%Y%m%d_%H%M%S");
}
