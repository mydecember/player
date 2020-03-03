#ifndef DEXCAMXM_VIDEODECODER_H
#define DEXCAMXM_VIDEODECODER_H

#include <stdint.h>
#include <string>
#include <vector>

using std::string;

struct AMediaCodec;
struct AMediaExtractor;

class VideoDecoder {
public:
    AMediaCodec * codec;
    AMediaExtractor * extractor;
    int trackIndex;
    int nFrame;
    size_t inputChunk;
    bool inputDone;
    bool outputDone;
    std::vector<uint8_t> frameBuffer;

    VideoDecoder();
    ~VideoDecoder();

    bool load(const string& path, int& W, int& H);

    /**
     * @param ts frame timestamp in ns, in media, not unix timestamp
     * @param data returned frame buffer, valid until next decodeFrame/release call
     * @param len returned frame buffer size
     * @return
     */
    bool decodeFrame(int64_t& ts, uint8_t** data, size_t& len);

    void release();
};


#endif //DEXCAMXM_VIDEODECODER_H
