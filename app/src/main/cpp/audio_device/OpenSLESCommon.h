//
// Created by zz on 20-4-1.
//

#ifndef TESTCODEC_OPENSLESCOMMON_H
#define TESTCODEC_OPENSLESCOMMON_H
#include <SLES/OpenSLES.h>
#include "SLES/OpenSLES_Android.h"
class OpenSLESCommon {

};
enum {
    kDefaultSampleRate = 44100,
    kDefaultBufSizeInSamples = kDefaultSampleRate * 10 / 1000,
};
#define OPENSLES_RECORDER_FAILED 0x80000001
#define OPENSLES_PLAYER_FAILED 0x80000002
#define kDefaultXiaomiDeviceSampleRate 16000

class PlayoutDelayProvider
{
public:
    virtual int PlayoutDelayMs() = 0;

protected:
    PlayoutDelayProvider() {}
    virtual ~PlayoutDelayProvider() {}
};

SLDataFormat_PCM CreatePcmConfiguration(int sample_rate, bool stereo);
#define CHECK_RESULT_GOTO_CLEANUP(res, msg) \
    { \
       if (!(res)) \
       { \
            Log("%s", msg.c_str()); \
            goto cleanup; \
       } \
    }

#endif //TESTCODEC_OPENSLESCOMMON_H
