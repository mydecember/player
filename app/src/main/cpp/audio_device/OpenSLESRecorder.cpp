//
// Created by zfq on 2020/3/31.
//

#include "OpenSLESRecorder.h"
#include "base/utils.h"
#define VOID_RETURN
#define OPENSL_SUCCESS(op) (((SLresult)op) == SL_RESULT_SUCCESS)
#define OPENSL_RETURN_ON_FAILURE(op, ret_val)                    \
  do {                                                           \
    SLresult err = (op);                                         \
    if (err != SL_RESULT_SUCCESS) {                              \
      Log("OpenSL error: %d" err);                               \
      assert(false);                                             \
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
}

OpenSLESRecorder::~OpenSLESRecorder() {

}

int32_t OpenSLESRecorder::InitMicrophone() {
    // Set up OpenSL engine.
    OPENSL_RETURN_ON_FAILURE(slCreateEngine(&sles_engine_, 1, kOption, 0,
                                            NULL, NULL),
                             -1);
    OPENSL_RETURN_ON_FAILURE((*sles_engine_)->Realize(sles_engine_,
                                                      SL_BOOLEAN_FALSE),
                             -1);
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
    audio_buffer_->SetRecordingSampleRate(rec_sampling_rate_);
    audio_buffer_->SetRecordingChannels(enable_stereo_ ? 2 : 1);
    UpdateRecordingDelay();
    AllocateBuffers();
    return 0;
}

void OpenSLESRecorder::UpdateSampleRate()
{
    // 0 is using the default value.
//    if (rec_sampling_rate_ == 0) {
//        rec_sampling_rate_ = kDefaultSampleRate;
//        LOG(LS_INFO, AUDIO_DEVICE_MODULE) << "Set the recording sample rate to be:" << rec_sampling_rate_ << std::endl;
//
//        if (DevicesSpecify::getInstance()->isTvMode()) {
//            LOG(LS_INFO, AUDIO_DEVICE_MODULE) << "For xiaomi machine, we set the sample rate to be the 16000 so we can enable hw AEC." << std::endl;
//            rec_sampling_rate_ = kDefaultXiaomiDeviceSampleRate;
//        } else if (audio_manager_.get() != NULL && audio_manager_->low_latency_supported()) {
//            rec_sampling_rate_ = audio_manager_->native_output_sample_rate();
//            LOG(LS_INFO, AUDIO_DEVICE_MODULE) << "Set the recording sample rate to be:" << rec_sampling_rate_ << " because of low latency support."<< std::endl;
//        }
//    }
//
//    LOG(LS_INFO, AUDIO_DEVICE_MODULE) << "Update the recording sample rate to be: " << (int)rec_sampling_rate_;
}

int32_t OpenSLESRecorder::StartRecording() {

}

int32_t OpenSLESRecorder::StopRecording() {

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
    CHECK_RESULT_GOTO_CLEANUP(AUDIO_DEVICE_MODULE, OPENSL_SUCCESS(res), "create audio recorder failed with:" << res);

    SLAndroidConfigurationItf configItf;
    /* Get the Android configuration interface which is explicit */
    res = (*sles_recorder_)->GetInterface(sles_recorder_, SL_IID_ANDROIDCONFIGURATION, (void*)&configItf);
    CHECK_RESULT_GOTO_CLEANUP(AUDIO_DEVICE_MODULE, OPENSL_SUCCESS(res), "create get the configuration interface failed with:" << res);

    force_mode = IniSettings::getInstance()->getString("webrtc/force_opensl_mic_mode", "");
    force_mode = string_tolower(force_mode);
    if ((DevicesSpecify::getInstance()->needSetModeToCommunication() && force_mode == "") || force_mode == "communication") {
        needToSetMode = true;
        voiceMode = SL_ANDROID_RECORDING_PRESET_VOICE_COMMUNICATION;
        LOG(LS_INFO, AUDIO_DEVICE_MODULE) << "Set the recording mode to voice communication to enable HW AEC ." << std::endl;
    } else if ((DevicesSpecify::getInstance()->needSetModeToRecognition() && force_mode == "") || force_mode == "recognition") {
        needToSetMode = true;
        voiceMode = SL_ANDROID_RECORDING_PRESET_VOICE_RECOGNITION;
        LOG(LS_INFO, AUDIO_DEVICE_MODULE) << "Set the recording mode to voice recognition to disable HW AEC ." << std::endl;
    } else if ((DevicesSpecify::getInstance()->needSetModeToNormal() && force_mode == "") || force_mode == "generic") {
        needToSetMode = true;
        voiceMode = SL_ANDROID_RECORDING_PRESET_GENERIC;
        LOG(LS_INFO, AUDIO_DEVICE_MODULE) << "Set the recording mode to generic to disable HW AEC ." << std::endl;
    }
    else {
        needToSetMode = true;
        voiceMode = SL_ANDROID_RECORDING_PRESET_VOICE_COMMUNICATION;
        LOG(LS_INFO, AUDIO_DEVICE_MODULE) << "default mode: Set the recording mode to voice communication to enable HW AEC ." << std::endl;
    }

    LOG(LS_INFO, AUDIO_DEVICE_MODULE) << " Set the recording mode before getSetting: " << voiceMode;
    {
        Emptyable<int> tmp = properties_.get<int>("opensl.recording.mode");
        if (!tmp.isEmpty()) {
            needToSetMode = true;
            voiceMode = tmp;
        }
    }

    LOG(LS_INFO, AUDIO_DEVICE_MODULE) << " Set the recording mode after getSetting: " << voiceMode;
    if (needToSetMode)
    {
        res = (*configItf)->SetConfiguration(configItf,SL_ANDROID_KEY_RECORDING_PRESET,&voiceMode, sizeof(SLuint32));
        CHECK_RESULT_GOTO_CLEANUP(AUDIO_DEVICE_MODULE, OPENSL_SUCCESS(res), "use hardware AEC failed with:" << res);
    }

    // Realize the recorder in synchronous mode.
    // This function may failed if other app (forground/background sensative) using the mic with an
    // different sample rate with error:SL_RESULT_CONTENT_UNSUPPORTED.
    res = (*sles_recorder_)->Realize(sles_recorder_, SL_BOOLEAN_FALSE);
    CHECK_RESULT_GOTO_CLEANUP(AUDIO_DEVICE_MODULE, OPENSL_SUCCESS(res), "realize recorder failed with:" << res);

    res = (*sles_recorder_)->GetInterface(sles_recorder_, SL_IID_RECORD, static_cast<void*>(&sles_recorder_itf_));
    CHECK_RESULT_GOTO_CLEANUP(AUDIO_DEVICE_MODULE, OPENSL_SUCCESS(res), "get record interface failed with:" << res);

    res = (*sles_recorder_)->GetInterface(sles_recorder_, SL_IID_ANDROIDSIMPLEBUFFERQUEUE, static_cast<void*>(&sles_recorder_sbq_itf_));
    CHECK_RESULT_GOTO_CLEANUP(AUDIO_DEVICE_MODULE, OPENSL_SUCCESS(res), "get recorder buffer queue interface failed with:" << res);
    return 0;

    cleanup:
    DestroyAudioRecorder();
    return OPENSLES_RECORDER_FAILED;
}





