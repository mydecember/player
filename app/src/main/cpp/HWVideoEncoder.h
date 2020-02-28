#ifndef DEXCAMXM_VIDEOENCODER_H
#define DEXCAMXM_VIDEOENCODER_H

#include <stdint.h>
#include <string>
#include <atomic>
#include <mutex>
#include <thread>
#include <memory>
#include <vector>
#include <deque>
#include <condition_variable>
#include "utils.h"

using std::string;
using std::vector;

struct AMediaCodec;
struct AMediaMuxer;

class HWVideoEncoder {
public:
    bool withAudio;
    AMediaCodec * videoCodec;
    AMediaCodec * audioCodec;
    AMediaMuxer * muxer;
    int fd;
    int mVideoTrack;
    int mAudioTrack;
    int32_t nFrame;
    int64_t lastFrameTs;

    int width;
    int height;
    int orientation;

    HWVideoEncoder();
    ~HWVideoEncoder();

    bool start(const string& path, int W, int H, int orientation = 0, bool withAudio=false);

    /**
     * @param ts frame timestamp in ns, in media, not unix timestamp
     * @param data frame buffer position
     * @param len frame buffer size
     */
    void encodeFrame(int64_t ts, uint8_t* data, size_t len);

    void encodeAudio(void* data, size_t len);

    void stop();

    void release();
};


class AsyncVideoEncoder {
public:
    AsyncVideoEncoder();
    ~AsyncVideoEncoder();

    bool start(const string& path, int W, int H, int orientation = 0);
    uint8_t* getBuffer(size_t& len);
    void encodeFrameRGB(int64_t ts, uint8_t* data);
    void stop();

private:
    size_t bufferSize();
    void run();

    string path;
    int width;
    int height;
    int orientation;

    HWVideoEncoder encoder;
    std::unique_ptr<std::thread> encodeThread;
    std::mutex lock;
    std::atomic<bool> running;

    struct EFrame {
        int64_t ts;
        uint8_t* data;
        EFrame() : ts(0),data(NULL) {}
        ~EFrame() {
            if (data) {
                free(data);
                data = NULL;
            }
        }
    };

    std::condition_variable hasFrame;
    std::deque<EFrame> queue;
};

#endif //DEXCAMXM_VIDEOENCODER_H
