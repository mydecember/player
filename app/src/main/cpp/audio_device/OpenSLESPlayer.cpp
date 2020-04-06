//
// Created by zfq on 2020/4/4.
//

#include "OpenSLESPlayer.h"
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

#define CHECK_RESULT_RETURN(res, result, msg) \
    { \
       if (!(res)) \
       { \
            Log("%s",msg); \
            return result; \
       } \
    }

//#define CHECK_RESULT_GOTO_CLEANUP(res, msg) \
//    { \
//       if (!(res)) \
//       { \
//            Log("%s", msg); \
//            goto cleanup; \
//       } \
//    }

static const SLEngineOption kOption[] = {
        { SL_ENGINEOPTION_THREADSAFE, static_cast<SLuint32>(SL_BOOLEAN_TRUE) },
};

OpenSLESPlayer::OpenSLESPlayer() {
    speaker_sampling_rate_ = 0;
    speaker_initialized_ = false;
    num_fifo_buffers_needed_  = 0;
    opensl_buffer_num_ = 2;
    enable_stereo_ = true;
    active_queue_ = 0;
    buffer_size_bytes_ = 0;
    buffer_size_samples_pre_channel_ = 0;
    playing_ = false;
    playout_delay_ = 0;
    play_initialized_ = false;
    input_file_ = fopen("/sdcard/reco.pcm", "rb");
    if (input_file_ == NULL) {
        LOG<<"create file error";
    }
}

OpenSLESPlayer::~OpenSLESPlayer() {

}

int32_t OpenSLESPlayer::InitSpeaker()
{
    // Set up OpenSl engine.
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
    // Set up OpenSl output mix.
    OPENSL_RETURN_ON_FAILURE(
            (*sles_engine_itf_)->CreateOutputMix(sles_engine_itf_,
                                                 &sles_output_mixer_,
                                                 0,
                                                 NULL,
                                                 NULL),
            -1);

    OPENSL_RETURN_ON_FAILURE(
            (*sles_output_mixer_)->Realize(sles_output_mixer_,
                                           SL_BOOLEAN_FALSE),
            -1);

    if (!InitSampleRate()) {
        Log("Init sample rate failed.");
        return -1;
    }
    speaker_initialized_ = true;
    return 0;
}

bool OpenSLESPlayer::InitSampleRate()
{
    //PrintLowLatencyInfo();
    if (speaker_sampling_rate_ == 0) {
        // Default is to use 10ms buffers.
        speaker_sampling_rate_ = kDefaultSampleRate;
        buffer_size_samples_pre_channel_ = speaker_sampling_rate_ * 10 / 1000;
        Log("Set the playback sample rate to be:%d",speaker_sampling_rate_ );
        if (!SetLowLatency()) {
            Log( "For xiaomi machine, we set the sample rate to be the 16000 so we can enable hw AEC." );
        }
    }
//    if (audio_buffer_->SetPlayoutSampleRate(speaker_sampling_rate_) < 0) {
//        return false;
//    }
//    if (audio_buffer_->SetPlayoutChannels(enable_stereo_ ? 2 : 1) < 0) {
//        return false;
//    }
    UpdatePlayoutDelay();
    AllocateBuffers();
    return true;
}

int32_t OpenSLESPlayer::SetStereoPlayout(bool enable) {
    Log("Set the playout as stereo:%d", enable);
    enable_stereo_ = enable;
    return 0;
}
void OpenSLESPlayer::UpdatePlayoutDelay()
{
    // TODO(hellner): Add accurate delay estimate.
    // On average half the current buffer will have been played out.
    int outstanding_samples = (TotalBuffersUsed() - 0.5) * buffer_size_samples_pre_channel_;
    playout_delay_ = outstanding_samples / (speaker_sampling_rate_ / 1000.0);
}

int OpenSLESPlayer::TotalBuffersUsed() const
{
    return num_fifo_buffers_needed_ + opensl_buffer_num_;
}
void OpenSLESPlayer::CalculateNumFifoBuffersNeeded()
{
    int number_of_bytes_needed =
            (speaker_sampling_rate_ * (enable_stereo_ ? 2 : 1) * sizeof(int16_t)) * 10 / 1000;
    // Ceiling of integer division: 1 + ((x - 1) / y)
    int buffers_per_10_ms =
            1 + ((number_of_bytes_needed - 1) / buffer_size_bytes_);
    // |num_fifo_buffers_needed_| is a multiple of 10ms of buffered up audio.
    num_fifo_buffers_needed_ = kNum10MsToBuffer * buffers_per_10_ms;
}
bool OpenSLESPlayer::SetLowLatency()
{
    buffer_size_samples_pre_channel_ = 960;
    speaker_sampling_rate_ = 44100;
    return true;
    // We may doesn't have javaVM for unittest, and audio_manager_ will be NULL
//    if (audio_manager_ == NULL) {
//        return false;
//    }
//
//    if (!audio_manager_->low_latency_supported()) {
//        return false;
//    }
//    buffer_size_samples_pre_channel_ = audio_manager_->native_buffer_size();
//    assert(buffer_size_samples_pre_channel_ > 0);
//    speaker_sampling_rate_ = audio_manager_->native_output_sample_rate();
//    assert(speaker_sampling_rate_ > 0);
//    return true;
}
int32_t OpenSLESPlayer::SetPlayoutSampleRate(uint32_t sample_rate_hz) {
    Log("Set the sample rate to be:%d", sample_rate_hz);
    speaker_sampling_rate_ = sample_rate_hz;
    buffer_size_samples_pre_channel_ = speaker_sampling_rate_ * 10 / 1000;
    return 0;
}

void OpenSLESPlayer::AllocateBuffers()
{
    // Allocate fine buffer to provide frames of the desired size.
    buffer_size_bytes_ = buffer_size_samples_pre_channel_ * sizeof(int16_t) * (enable_stereo_ ? 2 : 1);
//    fine_buffer_.reset(new FineAudioBuffer(audio_buffer_, buffer_size_bytes_,
//                                           speaker_sampling_rate_, enable_stereo_));

    // Allocate FIFO to handle passing buffers between processing and OpenSl
    // threads.
    CalculateNumFifoBuffersNeeded();  // Needs |buffer_size_bytes_| to be known
    assert(num_fifo_buffers_needed_ > 0);
//    fifo_.reset(new SingleRwFifo(num_fifo_buffers_needed_));

    // Allocate the memory area to be used.
    play_buf_.reset(new std::unique_ptr<int8_t[]>[TotalBuffersUsed()]);
//    int required_buffer_size = fine_buffer_->RequiredBufferSizeBytes();
    int required_buffer_size =buffer_size_bytes_;
    for (int i = 0; i < TotalBuffersUsed(); ++i) {
        play_buf_[i].reset(new int8_t[required_buffer_size]);
    }
}

bool OpenSLESPlayer::EnqueueAllBuffers()
{
    active_queue_ = 0;
    Log("Enqueue opensl buffer size:%d", buffer_size_bytes_);
    for (int i = 0; i < opensl_buffer_num_; ++i) {
        memset(play_buf_[i].get(), 0, buffer_size_bytes_);
        OPENSL_RETURN_ON_FAILURE(
                (*sles_player_sbq_itf_)->Enqueue(
                        sles_player_sbq_itf_,
                        reinterpret_cast<void*>(play_buf_[i].get()),
                        buffer_size_bytes_),
                false);
    }
    // OpenSL playing has been stopped. I.e. only this thread is touching
    // |fifo_|.
//    while (fifo_->size() != 0) {
//        // Underrun might have happened when pushing new buffers to the FIFO.
//        fifo_->Pop();
//    }
    for (int i = opensl_buffer_num_; i < TotalBuffersUsed(); ++i) {
        memset(play_buf_[i].get(), 0, buffer_size_bytes_);
//        fifo_->Push(play_buf_[i].get());
    }
    return true;
}

int OpenSLESPlayer::CreateAudioPlayer()
{
    Log("Create the opensl output player.");
    SLDataLocator_AndroidSimpleBufferQueue simple_buf_queue = {
            SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,
            static_cast<SLuint32>(opensl_buffer_num_)
    };
    SLDataFormat_PCM configuration = CreatePcmConfiguration(speaker_sampling_rate_, enable_stereo_);
    SLDataSource audio_source = { &simple_buf_queue, &configuration };

    SLDataLocator_OutputMix locator_outputmix;
    // Setup the data sink structure.
    locator_outputmix.locatorType = SL_DATALOCATOR_OUTPUTMIX;
    locator_outputmix.outputMix = sles_output_mixer_;
    SLDataSink audio_sink = { &locator_outputmix, NULL };

    // Interfaces for streaming audio data, setting volume and Android are needed.
    // Note the interfaces still need to be initialized. This only tells OpenSl
    // that the interfaces will be needed at some point.
    //
    // For android 7.0 with MIUI alpha version, it need SL_IID_EFFECTSEND, else there will be no audio for speaker mode.
    // and any mode change will not fix this. The sample code <native-audio> in android-ndk-r10d also requires SL_IID_EFFECTSEND in the code.
    // so let's add it, and should not hurt.
    SLInterfaceID ids[kNumInterfaces] = {
            SL_IID_BUFFERQUEUE, SL_IID_VOLUME, SL_IID_EFFECTSEND, SL_IID_ANDROIDCONFIGURATION
    };
    SLboolean req[kNumInterfaces] = {
            SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE
    };

    SLresult res = (*sles_engine_itf_)->CreateAudioPlayer(sles_engine_itf_, &sles_player_,
                                                          &audio_source, &audio_sink,
                                                          kNumInterfaces, ids, req);
    CHECK_RESULT_RETURN(OPENSL_SUCCESS(res), OPENSLES_PLAYER_FAILED, "Create audio player failed.");

    SLAndroidConfigurationItf playerConfig;
    res = (*sles_player_)->GetInterface(sles_player_, SL_IID_ANDROIDCONFIGURATION, &playerConfig);
    CHECK_RESULT_RETURN(OPENSL_SUCCESS(res), OPENSLES_PLAYER_FAILED, "Set configuration failed.");


    SLint32 streamType = SL_ANDROID_STREAM_VOICE;

    Log("Create the opensl output player. streamtype before:%d" ,streamType);
    res = (*playerConfig)->SetConfiguration(playerConfig, SL_ANDROID_KEY_STREAM_TYPE, &streamType, sizeof(SLint32));
    CHECK_RESULT_RETURN(OPENSL_SUCCESS(res), OPENSLES_PLAYER_FAILED, "Set configuration failed.");

    // Realize the player in synchronous mode.
    // This function may failed if other app (forground/background sensative) using the player.
    res = (*sles_player_)->Realize(sles_player_, SL_BOOLEAN_FALSE);
    CHECK_RESULT_GOTO_CLEANUP(OPENSL_SUCCESS(res), Format("realize player failed with:%d",res) );

    res = (*sles_player_)->GetInterface(sles_player_, SL_IID_PLAY, &sles_player_itf_);
    CHECK_RESULT_GOTO_CLEANUP( OPENSL_SUCCESS(res), Format("Get Play interface failed with:%d",res));

    res = (*sles_player_)->GetInterface(sles_player_, SL_IID_BUFFERQUEUE, &sles_player_sbq_itf_);
    CHECK_RESULT_GOTO_CLEANUP(OPENSL_SUCCESS(res), Format("Get the BufferQueue interface failed with:%d",res));

    return 0;

    cleanup:
    DestroyAudioPlayer();
    return OPENSLES_PLAYER_FAILED;
}

void OpenSLESPlayer::DestroyAudioPlayer()
{
    SLAndroidSimpleBufferQueueItf sles_player_sbq_itf = sles_player_sbq_itf_;
    {
        sles_player_sbq_itf_ = NULL;
        sles_player_itf_ = NULL;
    }
    if (sles_player_sbq_itf) {
        // Release all buffers currently queued up.
        OPENSL_RETURN_ON_FAILURE(
                (*sles_player_sbq_itf)->Clear(sles_player_sbq_itf),
                VOID_RETURN);
    }

    if (sles_player_) {
        (*sles_player_)->Destroy(sles_player_);
        sles_player_ = NULL;
    }
}

int32_t OpenSLESPlayer::StartPlayout()
{
    LOG << "OpenSlesOutput:Start playout" << std::endl;
    assert(!playing_);
    int res = CreateAudioPlayer();
    CHECK_RESULT_RETURN(res == 0, -1, "Create the audio player failed.");

    // Register callback to receive enqueued buffers.
    SLresult slRes = (*sles_player_sbq_itf_)->RegisterCallback(sles_player_sbq_itf_,
                                                               PlayerSimpleBufferQueueCallback,
                                                               this);
    CHECK_RESULT_RETURN(OPENSL_SUCCESS(slRes), -1, "Register the queue callback failed.");

    bool boolRes = EnqueueAllBuffers();
    CHECK_RESULT_RETURN(boolRes, -1, "Enqueue all buffer failed.");

    slRes = (*sles_player_itf_)->SetPlayState(sles_player_itf_, SL_PLAYSTATE_PLAYING);
    CHECK_RESULT_RETURN(OPENSL_SUCCESS(slRes), -1, "Set play state failed.");

    playing_ = true;
    return 0;
}

int32_t OpenSLESPlayer::StopPlayout()
{
    LOG<<"OpenSlesOutput:Stop playout" << std::endl;
    if (sles_player_itf_) {
        OPENSL_RETURN_ON_FAILURE(
                (*sles_player_itf_)->SetPlayState(sles_player_itf_,
                                                  SL_PLAYSTATE_STOPPED),
                -1);
    }

    DestroyAudioPlayer();
    playing_ = false;
    return 0;
}

void OpenSLESPlayer::PlayerSimpleBufferQueueCallback(
        SLAndroidSimpleBufferQueueItf sles_player_sbq_itf,
        void* p_context)
{
    uint32_t startTime = MilliTime();
    OpenSLESPlayer* audio_device = reinterpret_cast<OpenSLESPlayer*>(p_context);
    audio_device->PlayerSimpleBufferQueueCallbackHandler(sles_player_sbq_itf);
    uint32_t endTime = MilliTime();
    int usedTime = endTime - startTime;

    if (usedTime > audio_device->opensl_buffer_num_ * 10)
    {
        static int maxTime = 0;
        maxTime = std::max(usedTime, maxTime);
        LOG<<"The opensles audio play call back didn't finished in " << audio_device->opensl_buffer_num_*10<<"ms, and used:" << usedTime;
    }
}

void OpenSLESPlayer::PlayerSimpleBufferQueueCallbackHandler(
        SLAndroidSimpleBufferQueueItf sles_player_sbq_itf)
{
    LOG << "PlayerSimpleBufferQueueCallbackHandler function";
    //int8_t* audio = fifo_->Pop();
//    num_fifo_buffers_needed_
    int8_t* audio = play_buf_[active_queue_].get();
    int len = fread(audio, buffer_size_bytes_, 1, input_file_);
    if (feof(input_file_)) {
        rewind(input_file_);
    }
    active_queue_ = (active_queue_ + 1) % TotalBuffersUsed();
    OPENSL_RETURN_ON_FAILURE(
            (*sles_player_sbq_itf)->Enqueue(sles_player_sbq_itf,
                                            audio,
                                            buffer_size_bytes_),
            VOID_RETURN);
}