#include <unistd.h>
#include <fcntl.h>
#include <media/NdkMediaFormat.h>
#include <media/NdkMediaCodec.h>
#include <media/NdkMediaExtractor.h>
#include "VideoDecoder.h"

//#define Log printf
#include "base/utils.h"
#include <media/NdkMediaFormat.h>
#include <media/NdkMediaCodec.h>
#include <media/NdkMediaMuxer.h>
VideoDecoder::VideoDecoder() :
codec(NULL),
extractor(NULL),
trackIndex(-1),
nFrame(0),
inputChunk(0),
inputDone(true),
outputDone(true)
{
}

VideoDecoder::~VideoDecoder() {
    release();
}


bool VideoDecoder::load(const string &path, int &W, int &H) {
    Log("start load");
    extractor = AMediaExtractor_new();
    if (!extractor) {
        Log("extractor is error");
    }
    int fd = open(path.c_str(), O_RDONLY, 0444);
    if (fd <= 0) {
        Log("HWVideoencoder error !!!!!");
        return false;
    } else {
        Log("HWVideoencoder ok !!!!!");
    }

    off64_t filelen= lseek(fd,0L,SEEK_END);
    lseek(fd,0L,SEEK_SET);
    media_status_t status = AMediaExtractor_setDataSourceFd(extractor, fd, 0, filelen);
    //media_status_t status = AMediaExtractor_setDataSource(extractor, path.c_str());
    Log("to load path %s status %d len %lld ", path.c_str(), status, filelen);
    size_t ntrack = AMediaExtractor_getTrackCount(extractor);
    for (int i = 0; i < ntrack; i++) {
        AMediaFormat * format = AMediaExtractor_getTrackFormat(extractor, i);
        AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_WIDTH, &W);
        AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_HEIGHT, &H);
        const char * mine = NULL;
        AMediaFormat_getString(format, AMEDIAFORMAT_KEY_MIME, &mine);
        string smine(mine);
        if (StartsWith(smine, "video/")) {
            AMediaExtractor_selectTrack(extractor, i);
            Log("== decoder found video track: %d %s", i, smine.c_str());
            codec = AMediaCodec_createDecoderByType(smine.c_str());
            if (!codec) {
                Log("==== create codec failed: %s", smine.c_str());
                AMediaFormat_delete(format);
                release();
                return false;
            }
            media_status_t ret;
            ret = AMediaCodec_configure(codec, format, NULL, NULL, 0);
            if (ret != AMEDIA_OK) {
                Log("==== configure failed: %d", ret);
                release();
                return false;
            }
            AMediaFormat_delete(format);
            if ((ret = AMediaCodec_start(codec)) != AMEDIA_OK) {
                Log("== AMediaCodec_start failed: %d", ret);
                release();
                return false;
            };
            trackIndex = i;
            nFrame = 0;
            inputDone = false;
            outputDone = false;
            inputChunk = 0;
            return true;
        }
        AMediaFormat_delete(format);
    }
    Log("== decoder video track not found");
    release();
    return false;
}

bool VideoDecoder::decodeFrame(int64_t &ts, uint8_t **data, size_t &len) {
    while (!outputDone) {
        if (!inputDone) {
            int iIdx = AMediaCodec_dequeueInputBuffer(codec, -1);
            if (iIdx >= 0) {
                size_t ibufsize;
                uint8_t * ibuf = AMediaCodec_getInputBuffer(codec, iIdx, &ibufsize);
                ssize_t cnt = AMediaExtractor_readSampleData(extractor, ibuf, ibufsize);
                if (cnt < 0) {
                    //Log("== extractor reach EOF");
                    // End of Stream
                    AMediaCodec_queueInputBuffer(codec, iIdx, 0, 0, 0, AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM);
                    inputDone = true;
                } else {
                    int64_t tsUs = AMediaExtractor_getSampleTime(extractor);
                    //Log("== extract data size=%zd ts=%ld", cnt, tsUs);
                    ts = tsUs * 1000;
                    AMediaCodec_queueInputBuffer(codec, iIdx, 0, cnt, tsUs, 0);
                    AMediaExtractor_advance(extractor);
                    inputChunk++;
                }
            }
        }
        if (!outputDone) {
            AMediaCodecBufferInfo info;
            ssize_t oIdx = AMediaCodec_dequeueOutputBuffer(codec, &info, 0);
            if (oIdx == AMEDIACODEC_INFO_TRY_AGAIN_LATER) {
                Log("== AMEDIACODEC_INFO_TRY_AGAIN_LATER");
            } else if (oIdx == AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED) {
                Log("== AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED");
            } else if (oIdx < 0) {
                Log("== should not happen oIdx==%d", oIdx);
            } else {
                if (info.flags & AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM) {
                    // reach stream end
                    outputDone = true;
                }
                size_t obufsize = 0;
                uint8_t* obuf = AMediaCodec_getOutputBuffer(codec, oIdx, &obufsize);
                if (obuf && info.size > 0) {
                    if (frameBuffer.size() != info.size) {
                        frameBuffer.resize(info.size);
                    }
                    memcpy(frameBuffer.data(), obuf + info.offset, info.size);
                    AMediaCodec_releaseOutputBuffer(codec, oIdx, false);
                    *data = frameBuffer.data();
                    len = frameBuffer.size();
                    nFrame++;
                    return true;
                }
                AMediaCodec_releaseOutputBuffer(codec, oIdx, false);
            }
        }
    }
    release();
    return false;
}


void VideoDecoder::release() {
    inputDone = true;
    outputDone = true;
    if (codec) {
        AMediaCodec_stop(codec);
        AMediaCodec_delete(codec);
        codec = NULL;
    }
    if (extractor) {
        AMediaExtractor_delete(extractor);
        extractor = NULL;
    }
}
