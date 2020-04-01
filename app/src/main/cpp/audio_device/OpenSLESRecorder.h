//
// Created by zfq on 2020/3/31.
//

#ifndef TESTCODEC_OPENSLESRECORDER_H
#define TESTCODEC_OPENSLESRECORDER_H
#include "OpenSLESCommon.h"
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <SLES/OpenSLES_AndroidConfiguration.h>

class OpenSLESRecorder {
public:
    OpenSLESRecorder();
    ~OpenSLESRecorder();
    int32_t InitMicrophone();
    int32_t StartRecording();
    int32_t StopRecording();


private:
    enum {
        kNumInterfaces = 2,
        // Keep as few OpenSL buffers as possible to avoid wasting memory. 2 is
        // minimum for playout. Keep 2 for recording as well.
                kNumOpenSlBuffers = 10,
        kNum10MsToBuffer = 1,
    };
    int CreateAudioRecorder();
    int TotalBuffersUsed() const;
    void CalculateNumFifoBuffersNeeded();
    int InitSampleRate();
    void UpdateSampleRate();

private:
    // OpenSL handles
    SLObjectItf sles_engine_;
    SLEngineItf sles_engine_itf_;
    SLObjectItf sles_recorder_;
    SLRecordItf sles_recorder_itf_;
    SLAndroidSimpleBufferQueueItf sles_recorder_sbq_itf_;
    //IAudioDeviceBuffer* audio_buffer_;

    int num_fifo_buffers_needed_;
    int opensl_buffer_num_;
    uint32_t rec_sampling_rate_;
    bool enable_stereo_;
};


#endif //TESTCODEC_OPENSLESRECORDER_H
