//
// Created by zz on 20-4-1.
//

#include "OpenSLESCommon.h"
SLDataFormat_PCM CreatePcmConfiguration(int sample_rate, bool stereo)
{
    SLDataFormat_PCM configuration;
    configuration.formatType = SL_DATAFORMAT_PCM;
    configuration.numChannels = (stereo ? 2 : 1);
    // According to the opensles documentation in the ndk:
    // samplesPerSec is actually in units of milliHz, despite the misleading name.
    // It further recommends using constants. However, this would lead to a lot
    // of boilerplate code so it is not done here.
    configuration.samplesPerSec = sample_rate * 1000;
    configuration.bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_16;
    configuration.containerSize = SL_PCMSAMPLEFORMAT_FIXED_16;
    configuration.channelMask = SL_SPEAKER_FRONT_CENTER;
    if (2 == configuration.numChannels) {
        configuration.channelMask =
                SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
    }
    configuration.endianness = SL_BYTEORDER_LITTLEENDIAN;
    return configuration;
}