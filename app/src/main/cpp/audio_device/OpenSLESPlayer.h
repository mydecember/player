//
// Created by zfq on 2020/4/4.
//
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <SLES/OpenSLES_AndroidConfiguration.h>
#include <memory>
#include "OpenSLESCommon.h"
#ifndef TESTCODEC_OPENSLESPLAYER_H
#define TESTCODEC_OPENSLESPLAYER_H


class OpenSLESPlayer {
public:
    OpenSLESPlayer();
    ~OpenSLESPlayer();
    int32_t InitSpeaker();
    int32_t SetStereoPlayout(bool enable);
    int32_t StartPlayout();
    int32_t SetPlayoutSampleRate(uint32_t sample_rate_hz);
    int32_t StopPlayout();
private:
    enum {
        kNumInterfaces = 4,
        // TODO(xians): Reduce the numbers of buffers to improve the latency.
        //              Currently 30ms worth of buffers are needed due to audio
        //              pipeline processing jitter. Note: kNumOpenSlBuffers must
        //              not be changed.
        // According to the opensles documentation in the ndk:
        // The lower output latency path is used only if the application requests a
        // buffer count of 2 or more. Use minimum number of buffers to keep delay
        // as low as possible.
                kNumOpenSlBuffers = 10,
        // NetEq delivers frames on a 10ms basis. This means that every 10ms there
        // will be a time consuming task. Keeping 10ms worth of buffers will ensure
        // that there is 10ms to perform the time consuming task without running
        // into underflow.
        // In addition to the 10ms that needs to be stored for NetEq processing
        // there will be jitter in audio pipe line due to the acquisition of locks.
        // Note: The buffers in the OpenSL queue do not count towards the 10ms of
        // frames needed since OpenSL needs to have them ready for playout.
                kNum10MsToBuffer = 1,
    };
    bool InitSampleRate();
    bool SetLowLatency();
    void UpdatePlayoutDelay();
    int TotalBuffersUsed() const;
    void CalculateNumFifoBuffersNeeded();
    void AllocateBuffers();
    bool EnqueueAllBuffers();
    int CreateAudioPlayer();
    void DestroyAudioPlayer();
    static void PlayerSimpleBufferQueueCallback(
            SLAndroidSimpleBufferQueueItf sles_player_sbq_itf,
            void* p_context);
    void PlayerSimpleBufferQueueCallbackHandler(
            SLAndroidSimpleBufferQueueItf queueItf);
private:
    // OpenSL handles
    SLObjectItf sles_engine_;
    SLEngineItf sles_engine_itf_;
    SLObjectItf sles_player_;
    SLPlayItf sles_player_itf_;
    SLAndroidSimpleBufferQueueItf sles_player_sbq_itf_;
    SLObjectItf sles_output_mixer_;

    // Audio settings
    uint32_t speaker_sampling_rate_;
    int buffer_size_samples_pre_channel_;
    int buffer_size_bytes_;

    // Audio status
    uint16_t playout_delay_;
    int opensl_buffer_num_;
    bool enable_stereo_;
    bool speaker_initialized_;
    int num_fifo_buffers_needed_;
    int active_queue_;
    bool playing_;
    bool play_initialized_;
    std::unique_ptr<std::unique_ptr<int8_t[]>[]> play_buf_;
    FILE* input_file_;
};


#endif //TESTCODEC_OPENSLESPLAYER_H
