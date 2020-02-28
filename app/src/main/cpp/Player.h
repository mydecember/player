//
// Created by zz on 20-2-28.
//

#ifndef TESTCODEC_PLAYER_H
#define TESTCODEC_PLAYER_H
#include <string>
#include "Demuxer.h"
#include "thread"
#include "syslog.h"
#include <android/log.h>
#include "utils.h"
class Player {
public:
    Player();
    ~Player();
    int Open(std::string url);
    int SetSurface();
    int Start();
    int Pause();
    int Close();
private:
    std::string url_;
};


#endif //TESTCODEC_PLAYER_H
