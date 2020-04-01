//
// Created by zz on 20-4-1.
//

#ifndef TESTCODEC_OPENSLESCOMMON_H
#define TESTCODEC_OPENSLESCOMMON_H
#include <SLES/OpenSLES.h>

class OpenSLESCommon {

};
enum {
    kDefaultSampleRate = 44100,
    kDefaultBufSizeInSamples = kDefaultSampleRate * 10 / 1000,
};

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

#endif //TESTCODEC_OPENSLESCOMMON_H
