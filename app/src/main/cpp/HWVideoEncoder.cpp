//
// Created by zz on 20-2-28.
//

#include "HWVideoEncoder.h"
#include <unistd.h>
#include <fcntl.h>
#include <media/NdkMediaFormat.h>
#include <media/NdkMediaCodec.h>
#include <media/NdkMediaMuxer.h>
#include <queue>


HWVideoEncoder::HWVideoEncoder() :
        videoCodec(NULL),audioCodec(NULL), muxer(NULL),
        mVideoTrack(-1), mAudioTrack(-1), fd(-1), nFrame(0), lastFrameTs(0), orientation(0),
        withAudio(false) {
}

HWVideoEncoder::~HWVideoEncoder() {
    release();
}

void HWVideoEncoder::release() {
    mVideoTrack = -1;
    if (videoCodec) {
        AMediaCodec_delete(videoCodec);
        videoCodec = NULL;
    }
    if (audioCodec) {
        AMediaCodec_delete(audioCodec);
        audioCodec = NULL;
    }
    if (muxer) {
        AMediaMuxer_delete(muxer);
        muxer = NULL;
    }
    if (fd >= 0) {
        close(fd);
        fd = -1;
    }
}

static int calcBitrate(int H, int W) {
    return H * W * 32;
}

bool HWVideoEncoder::start(const string& path, int W, int H, int orientation, bool withAudio) {
    fd = open(path.c_str(), O_CREAT | O_WRONLY, 0644);
    if (fd == -1) {
        Log("HWVideoencoder error !!!!!");
        return false;
    }

    this->withAudio = withAudio;
    this->orientation = orientation;

    const char * vmine = "video/avc";
    AMediaFormat *videoFormat = AMediaFormat_new();
    AMediaFormat_setString(videoFormat, AMEDIAFORMAT_KEY_MIME, vmine);
    AMediaFormat_setInt32(videoFormat, AMEDIAFORMAT_KEY_WIDTH, W);
    AMediaFormat_setInt32(videoFormat, AMEDIAFORMAT_KEY_HEIGHT, H);
    AMediaFormat_setInt32(videoFormat, AMEDIAFORMAT_KEY_BIT_RATE, calcBitrate(H, W));
    //AMediaFormat_setInt32(videoFormat, AMEDIAFORMAT_KEY_BITRATE_MODE, 1);
    AMediaFormat_setInt32(videoFormat, AMEDIAFORMAT_KEY_FRAME_RATE, 30);
    AMediaFormat_setInt32(videoFormat, AMEDIAFORMAT_KEY_I_FRAME_INTERVAL, 5);
    const int COLOR_FormatYUV420Flexible            = 0x7F420888;
    const int COLOR_FormatRGBFlexible               = 0x7F36B888;// not supported
    //AMediaFormat_setInt32(videoFormat, AMEDIAFORMAT_KEY_COLOR_FORMAT, COLOR_FormatYUV420Flexible);
    // seems not necessary
//        uint8_t sps[2] = {0x12, 0x12};
//        uint8_t pps[2] = {0x12, 0x12};
//        AMediaFormat_setBuffer(videoFormat, "csd-0", sps, 2); // sps
//        AMediaFormat_setBuffer(videoFormat, "csd-1", pps, 2); // pps
    videoCodec = AMediaCodec_createEncoderByType(vmine);
    if (!videoCodec) {
        Log("==== create codec failed: %s", vmine);
        return false;
    }
    media_status_t ret;
    ret = AMediaCodec_configure(videoCodec, videoFormat, NULL, NULL,
                                AMEDIACODEC_CONFIGURE_FLAG_ENCODE);
    if (ret != AMEDIA_OK) {
        Log("==== configure failed: %d", ret);
        AMediaFormat_delete(videoFormat);
        release();
        return false;
    }
    AMediaFormat_delete(videoFormat);

    if (withAudio) {
        const char * amine = "audio/mp4a-latm";
        AMediaFormat *audioFormat = AMediaFormat_new();
        AMediaFormat_setString(audioFormat, AMEDIAFORMAT_KEY_MIME, amine);
        AMediaFormat_setInt32(audioFormat, AMEDIAFORMAT_KEY_SAMPLE_RATE, 44100);
        AMediaFormat_setInt32(audioFormat, AMEDIAFORMAT_KEY_AAC_PROFILE, 2);
        AMediaFormat_setInt32(audioFormat, AMEDIAFORMAT_KEY_BIT_RATE, 128000);
        AMediaFormat_setInt32(audioFormat, AMEDIAFORMAT_KEY_CHANNEL_COUNT, 1);
        AMediaFormat_setInt32(audioFormat, AMEDIAFORMAT_KEY_IS_ADTS, 0);
        uint8_t es[2] = {0x12, 0x12};
        AMediaFormat_setBuffer(audioFormat, "csd-0", es, 2);
        audioCodec = AMediaCodec_createEncoderByType(amine);
        media_status_t audioConfigureStatus = AMediaCodec_configure(audioCodec, audioFormat, NULL, NULL,
                                                                    AMEDIACODEC_CONFIGURE_FLAG_ENCODE);
        if (AMEDIA_OK != audioConfigureStatus) {
            Log("set audio configure failed status-->%d", audioConfigureStatus);
            return false;
        }
        AMediaFormat_delete(audioFormat);
    }

    nFrame = 0;
    lastFrameTs = 0;
    muxer = AMediaMuxer_new(fd, AMEDIAMUXER_OUTPUT_FORMAT_MPEG_4);
    mVideoTrack = -1;
    if ((ret = AMediaCodec_start(videoCodec)) != AMEDIA_OK) {
        Log("== AMediaCodec_start video failed: %d", ret);
    };
    mAudioTrack = -1;
    if (withAudio) {
        if ((ret = AMediaCodec_start(audioCodec)) != AMEDIA_OK) {
            Log("== AMediaCodec_start audio failed: %d", ret);
        };
    }
    return true;
}

void HWVideoEncoder::stop() {
    if (videoCodec) {
        encodeFrame(lastFrameTs, NULL, 0);
        AMediaCodec_stop(videoCodec);
    }
    if (muxer) {
        AMediaMuxer_stop(muxer);
    }
    release();
}

/**
 * @param ts frame timestamp in ns, in media, not unix timestamp
 * @param data frame buffer position
 * @param len frame buffer size
 */
void HWVideoEncoder::encodeFrame(int64_t ts, uint8_t* data, size_t len) {
    if (!videoCodec) {
        Log("=====encodeFrame failed: codec closed");
        return;
    }
    bool flush = (data == NULL);
    ssize_t index = AMediaCodec_dequeueInputBuffer(videoCodec, -1);
    size_t ibufsize = 0;
    if (index >= 0) {
        uint8_t * ibuf = AMediaCodec_getInputBuffer(videoCodec, index, &ibufsize);
        if (ibufsize > 0) {
            if (len > ibufsize) {
                Log("len(%zu) > ibufsize(%zu)", len, ibufsize);
                return;
            }
            if (data) {
                //Log("== input %zu  ibufsize %zu", len, ibufsize);
                memcpy(ibuf, data, len);
                AMediaCodec_queueInputBuffer(videoCodec, index, 0, len, ts / 1000, 0);
                nFrame++;
                lastFrameTs = ts;
            } else {
                //Log("== input EOS");
                AMediaCodec_queueInputBuffer(videoCodec, index, 0, 0, ts / 1000, AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM);
            }
        }
    } else {
        Log("dequeue input  error");
    }
    while (true) {
        AMediaCodecBufferInfo info;
        memset(&info, 0, sizeof(info));
        ssize_t outIndex = AMediaCodec_dequeueOutputBuffer(videoCodec, &info, flush ? 1000 : 0);
        if (outIndex == AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED) {
            AMediaFormat * format = AMediaCodec_getOutputFormat(videoCodec);
            if (mVideoTrack != -1) {
                Log("== ilegal state, mtrack != -1");
            } else {
                Log("== setup & start muxer");
                media_status_t oret = AMediaMuxer_setOrientationHint(muxer, orientation);
                if (oret != AMEDIA_OK) {
                    Log("=== set orientation error %d", oret);
                }
                mVideoTrack = AMediaMuxer_addTrack(muxer, format);
                if (mVideoTrack < 0) {
                    Log("=== addTrack failed: %zu", mVideoTrack);
                    return;
                }
                int ret;
                if ((ret = AMediaMuxer_start(muxer)) != AMEDIA_OK) {
                    Log("=== AMediaMuxer_start failed: %d", ret);
                    return;
                }
            }
            break;
        } else if (outIndex < 0 && !flush) {
            break;
        }
        size_t obufsize = 0;
        uint8_t* obuf = AMediaCodec_getOutputBuffer(videoCodec, outIndex, &obufsize);

        if ((info.flags & AMEDIACODEC_BUFFER_FLAG_CODEC_CONFIG) != 0) {
            info.size = 0;
        }

        if (info.size > 0 ) {
            if (obuf && mVideoTrack >= 0) {
                //Log("=== out %d", info.size);
                media_status_t ret;
                if ((ret = AMediaMuxer_writeSampleData(muxer, mVideoTrack, obuf, &info)) != 0) {
                    Log("== error muxer_writeSampleData: %d", ret);
                }
            } else {
                Log("==error obuf=%p NULL or mTrack=%zd <0 info.size=%d", obuf, mVideoTrack, info.size);
            }
        }

        if (flush && ((info.flags & AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM) != 0)) {
            AMediaCodec_releaseOutputBuffer(videoCodec, outIndex, false);
            break;
        }

        AMediaCodec_releaseOutputBuffer(videoCodec, outIndex, false);
    }
}


void HWVideoEncoder::encodeAudio(void* data, size_t len) {
    // TODO:
}


///////////////////////////////////////////////////////////////////////////////

AsyncVideoEncoder::AsyncVideoEncoder() : running(false), width(0), height(0), orientation(0) {

}

AsyncVideoEncoder::~AsyncVideoEncoder() {
    if (encodeThread) {
        Log("AsyncVideoEncoder have not call stop before cleanup");
        exit(1);
    }
}

bool AsyncVideoEncoder::start(const string& path, int W, int H, int orientation) {
    if (encodeThread) {
        Log("start when thread already running");
        exit(1);
        return false;
    }
    this->path = path;
    this->width = W;
    this->height = H;
    this->orientation = orientation;
    running = true;
    encodeThread.reset(new std::thread(&AsyncVideoEncoder::run, this));
    return true;
}

uint8_t* AsyncVideoEncoder::getBuffer(size_t& len) {
    len = bufferSize();
    return (uint8_t*)malloc(len);
}

void AsyncVideoEncoder::encodeFrameRGB(int64_t ts, uint8_t* data) {
    std::lock_guard<std::mutex> lg(lock);
    queue.emplace_back();
    EFrame& f = queue.back();
    f.ts = ts;
    f.data = data;
    hasFrame.notify_one();
}

void AsyncVideoEncoder::stop() {
    if (!encodeThread) {
        Log("stop called when no thread running");
        return;
    }
    {
        std::lock_guard<std::mutex> lg(lock);
        running = false;
        hasFrame.notify_one();
    }
    encodeThread->join();
    encodeThread.reset(NULL);
}

size_t AsyncVideoEncoder::bufferSize() {
    return width * height * 3;
}

void AsyncVideoEncoder::run() {
    encoder.start(path, width, height, orientation);
    while (true) {
        EFrame nf;
        {
            std::unique_lock<std::mutex> ul(lock);
            while (queue.empty() && running) {
                hasFrame.wait(ul);
            }
            if (!queue.empty()) {
                std::swap(nf, queue.front());
                queue.pop_front();
            }
        }
        if (!nf.data) {
            break;
        }

       encoder.encodeFrame(nf.ts, nf.data, height*width*3/2);
    }
    encoder.stop();
}
