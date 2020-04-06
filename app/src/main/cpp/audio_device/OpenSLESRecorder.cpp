//
// Created by zfq on 2020/3/31.
//

#include <unistd.h>
#include "OpenSLESRecorder.h"
#include "base/utils.h"
#define VOID_RETURN
#define OPENSL_SUCCESS(op) (((SLresult)op) == SL_RESULT_SUCCESS)
#define OPENSL_RETURN_ON_FAILURE(op, ret_val)                    \
  do {                                                           \
    SLresult err = (op);                                         \
    if (err != SL_RESULT_SUCCESS) {                              \
      Log("OpenSL error: %d",  err);                 \
      return ret_val;                                            \
    }                                                            \
  } while (0)

static const SLEngineOption kOption[] = {
        { SL_ENGINEOPTION_THREADSAFE, static_cast<SLuint32>(SL_BOOLEAN_TRUE) },
};
OpenSLESRecorder::OpenSLESRecorder() {
    num_fifo_buffers_needed_ = kNum10MsToBuffer;
    opensl_buffer_num_ = 2;
    rec_sampling_rate_ = 48000;
    enable_stereo_ = true;
    mic_initialized_ = false;
    recording_delay_ = 0;
    active_queue_ = 0;
    recording_ = false;
    number_callback_time_exceeded_ = 0;
    sles_engine_ = nullptr;
    sles_engine_itf_ = nullptr;
    sles_recorder_ = nullptr;
    sles_recorder_itf_ = nullptr;
}

OpenSLESRecorder::~OpenSLESRecorder() {

}

int32_t OpenSLESRecorder::InitMicrophone() {
    // Set up OpenSL engine.
    OPENSL_RETURN_ON_FAILURE(slCreateEngine(&sles_engine_, 1, kOption, 0, NULL, NULL), -1);
    OPENSL_RETURN_ON_FAILURE((*sles_engine_)->Realize(sles_engine_,SL_BOOLEAN_FALSE), -1);
    OPENSL_RETURN_ON_FAILURE((*sles_engine_)->GetInterface(sles_engine_,
                                                           SL_IID_ENGINE,
                                                           &sles_engine_itf_),
                             -1);

    if (InitSampleRate() != 0) {
        return -1;
    }
    mic_initialized_ = true;
    return 0;
}

int OpenSLESRecorder::InitSampleRate()
{
    UpdateSampleRate();
//    audio_buffer_->SetRecordingSampleRate(rec_sampling_rate_);
//    audio_buffer_->SetRecordingChannels(enable_stereo_ ? 2 : 1);
    UpdateRecordingDelay();
    AllocateBuffers();
    return 0;
}

void OpenSLESRecorder::UpdateRecordingDelay()
{
    // TODO(hellner): Add accurate delay estimate.
    // On average half the current buffer will have been filled with audio.
    int outstanding_samples =
            (TotalBuffersUsed() - 0.5) * buffer_size_samples_pre_channel();
    recording_delay_ = (int)(outstanding_samples / (rec_sampling_rate_ / 1000.0));
}

int OpenSLESRecorder::buffer_size_samples_pre_channel() const
{
    // Since there is no low latency recording, use buffer size corresponding to
    // 10ms of data since that's the framesize WebRTC uses. Getting any other
    // size would require patching together buffers somewhere before passing them
    // to WebRTC.
    return rec_sampling_rate_ * 20 / 1000;
}

void OpenSLESRecorder::UpdateSampleRate()
{
    rec_sampling_rate_ = kDefaultSampleRate;
    Log("pdate the recording sample rate to be: %d", (int)rec_sampling_rate_);
}

int32_t OpenSLESRecorder::StartRecording() {
    int res = CreateAudioRecorder();
    if (res != SL_RESULT_SUCCESS) {
       Log("create audio recoder error %d", res);
    }
    res =  (*sles_recorder_sbq_itf_)->RegisterCallback(sles_recorder_sbq_itf_, RecorderSimpleBufferQueueCallback, this);
    if (res != SL_RESULT_SUCCESS) {
        Log("RegisterCallback error %d", res);
        return -1;
    }
    if (!EnqueueAllBuffers()) {
        return -1;
    }
    recording_ = true;
    res = (*sles_recorder_itf_)->SetRecordState(sles_recorder_itf_, SL_RECORDSTATE_RECORDING);
    if (res != SL_RESULT_SUCCESS) {
        Log("SetRecordState error");
        return -1;
    }
    Log("StartRecording finished.");
    return 0;
}

int32_t OpenSLESRecorder::StopRecording() {
    Log("Set record state to stopped.threadID:");
    if (sles_recorder_itf_) {
        OPENSL_RETURN_ON_FAILURE(
                (*sles_recorder_itf_)->SetRecordState(sles_recorder_itf_,
                                                      SL_RECORDSTATE_STOPPED),
                -1);
    }
    Log( "Try to destory the recorder.threadID:");
    // by zijwu, we sleep 200 to avoid deadlock inside the OpenSLES.
    // The bug link:https://android-review.googlesource.com/#/c/105606/
    // It will happen when the hardware send data to callback, and at the same time
    // we try to destroy the recorder. So the solution is sleep 200ms to make sure the
    // OpenSLES callback thread has stopped, then we try to destroy the recorder.
    usleep(200000);
    DestroyAudioRecorder();
    recording_ = false;

    Log("StopRecording finished.");
    return 0;
}

void OpenSLESRecorder::AllocateBuffers() {
    // Allocate the memory area to be used.
    rec_buf_.reset(new std::unique_ptr<int8_t[]>[TotalBuffersUsed()]);
    for (int i = 0; i < TotalBuffersUsed(); ++i) {
        rec_buf_[i].reset(new int8_t[buffer_size_bytes()]);
    }
}

bool OpenSLESRecorder::EnqueueAllBuffers() {
    active_queue_ = 0;
    for (int i = 0; i < opensl_buffer_num_; ++i) {
        memset(rec_buf_[i].get(), 0, buffer_size_bytes());
        int res = (*sles_recorder_sbq_itf_)->Enqueue(sles_recorder_sbq_itf_,
                reinterpret_cast<void*>(rec_buf_[i].get()),
                buffer_size_bytes());
        if (res != SL_RESULT_SUCCESS ) {
            Log("enqueue error");
            return false;
        }
    }
    return true;
}

int OpenSLESRecorder::buffer_size_samples_pre_channel() {
    return rec_sampling_rate_ * 10 / 1000;
}

int OpenSLESRecorder::buffer_size_bytes() const
{
    return buffer_size_samples_pre_channel() * (enable_stereo_ ? 2 : 1) * sizeof(int16_t);
}

void OpenSLESRecorder::RecorderSimpleBufferQueueCallback(
        SLAndroidSimpleBufferQueueItf queue_itf,
        void* context) {
    OpenSLESRecorder* audio_device = reinterpret_cast<OpenSLESRecorder*>(context);
    uint32_t startTime = MilliTime();
    audio_device->RecorderSimpleBufferQueueCallbackHandler(queue_itf);
    uint32_t endTime = MilliTime();
    int usedTime = endTime - startTime;

    if (usedTime > audio_device->opensl_buffer_num_ * 10)
    {
        static int maxTime = 0;
        maxTime = std::max(maxTime, usedTime);
       Log("The opensles audio record call back didn't finished in %lld ms, and used: %d, number_callback_time_exceeded_ %d maxTime %d",
               audio_device->opensl_buffer_num_*10,
               usedTime,
               audio_device->number_callback_time_exceeded_,
               maxTime);
        audio_device->number_callback_time_exceeded_ ++;
    }
}

void OpenSLESRecorder::RecorderSimpleBufferQueueCallbackHandler(SLAndroidSimpleBufferQueueItf queueItf) {
    // There is at least one spot available in the fifo.
    int8_t* audio = rec_buf_[active_queue_].get();

    static int64_t pre = 0;
    int64_t now = MilliTime();
    Log("get audio %lld samples used %lld  sample reate %d",
            buffer_size_samples_pre_channel(),
            (now - pre),
            rec_sampling_rate_);
    pre = now;
    DECLARE_DUMP_FILE("/sdcard/reco.pcm", dumpfile);
    DUMP_UTIL_FILE_WRITE(dumpfile, audio, buffer_size_bytes());

    active_queue_ = (active_queue_ + 1) % TotalBuffersUsed();
    int next_free_buffer =
            (active_queue_ + opensl_buffer_num_ - 1) % TotalBuffersUsed();
    int res = (*sles_recorder_sbq_itf_)->Enqueue(
            sles_recorder_sbq_itf_,
            reinterpret_cast<void*>(rec_buf_[next_free_buffer].get()),
            buffer_size_bytes());
    if (res != SL_RESULT_SUCCESS) {
        Log("Enqueue error next_free_buffer %d", next_free_buffer);
        return;
    }
}

int OpenSLESRecorder::TotalBuffersUsed() const
{
  return num_fifo_buffers_needed_ + opensl_buffer_num_;
}

void OpenSLESRecorder::CalculateNumFifoBuffersNeeded()
{
    // Buffer size is 10ms of data.
    num_fifo_buffers_needed_ = kNum10MsToBuffer;
}

int OpenSLESRecorder::CreateAudioRecorder() {
    // will be set on Xiaomi machine to enable HW AEC.
    SLuint32 voiceMode = SL_ANDROID_RECORDING_PRESET_VOICE_COMMUNICATION;
    SLDataLocator_IODevice micLocator = {
            SL_DATALOCATOR_IODEVICE, SL_IODEVICE_AUDIOINPUT,
            SL_DEFAULTDEVICEID_AUDIOINPUT, NULL
    };
    SLDataSource audio_source = { &micLocator, NULL };

    SLDataLocator_AndroidSimpleBufferQueue simple_buf_queue = {
            SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,
            static_cast<SLuint32>(TotalBuffersUsed())
    };
    SLDataFormat_PCM configuration =
            CreatePcmConfiguration(rec_sampling_rate_, enable_stereo_);
    SLDataSink audio_sink = { &simple_buf_queue, &configuration };

    // Interfaces for recording android audio data and Android are needed.
    // Note the interfaces still need to be initialized. This only tells OpenSl
    // that the interfaces will be needed at some point.
    const SLInterfaceID id[kNumInterfaces] = {
            SL_IID_ANDROIDSIMPLEBUFFERQUEUE, SL_IID_ANDROIDCONFIGURATION
    };
    const SLboolean req[kNumInterfaces] = {
            SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE
    };

    bool needToSetMode = false;
    std::string force_mode;

    SLresult res = (*sles_engine_itf_)->CreateAudioRecorder(sles_engine_itf_,
                                                            &sles_recorder_, &audio_source, &audio_sink, kNumInterfaces, id, req);
    if (res != SL_RESULT_SUCCESS) {
        Log("create audio recorder failed with %d", res);
        goto cleanup;
    }

    SLAndroidConfigurationItf configItf;
    /* Get the Android configuration interface which is explicit */
    res = (*sles_recorder_)->GetInterface(sles_recorder_, SL_IID_ANDROIDCONFIGURATION, (void*)&configItf);
    if (res != SL_RESULT_SUCCESS) {
        Log("create get the configuration interface failed with %d", res);
        goto cleanup;
    }

    needToSetMode = true;
    voiceMode = SL_ANDROID_RECORDING_PRESET_VOICE_COMMUNICATION;

    Log(" Set the recording mode after getSetting: " , voiceMode);
    if (needToSetMode)
    {
        res = (*configItf)->SetConfiguration(configItf,SL_ANDROID_KEY_RECORDING_PRESET,&voiceMode, sizeof(SLuint32));
        if (res != SL_RESULT_SUCCESS) {
            Log("use hardware AEC failed with %d", res);
            goto cleanup;
        }
    }

    // Realize the recorder in synchronous mode.
    // This function may failed if other app (forground/background sensative) using the mic with an
    // different sample rate with error:SL_RESULT_CONTENT_UNSUPPORTED.
    res = (*sles_recorder_)->Realize(sles_recorder_, SL_BOOLEAN_FALSE);
    if (res != SL_RESULT_SUCCESS) {
        Log("realize recorder failed with %d", res);
        goto cleanup;
    }

    res = (*sles_recorder_)->GetInterface(sles_recorder_, SL_IID_RECORD, static_cast<void*>(&sles_recorder_itf_));
    if (res != SL_RESULT_SUCCESS) {
        Log("get record interface failed with %d", res);
        goto cleanup;
    }

    res = (*sles_recorder_)->GetInterface(sles_recorder_, SL_IID_ANDROIDSIMPLEBUFFERQUEUE, static_cast<void*>(&sles_recorder_sbq_itf_));
    if (res != SL_RESULT_SUCCESS) {
        Log("get recorder buffer queue interface failed with %d", res);
        goto cleanup;
    }
    Log("CreateAudioRecorder success");
    return 0;
    cleanup:
    DestroyAudioRecorder();
    return -1;
}

void OpenSLESRecorder::DestroyAudioRecorder() {

}




