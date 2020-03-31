//
// Created by zfq on 2020/3/14.
//

#include "Muxer.h"
#include "UERead.h"
#include<mutex>
#include<thread>
#include <base/utils.h>
#include <unistd.h>

#define LOG_BUF_PREFIX_SIZE 512
#define LOG_BUF_SIZE 2048
static char logBufPrefix[LOG_BUF_PREFIX_SIZE];
static char logBuffer[LOG_BUF_SIZE];
static pthread_mutex_t cb_av_log_lock;
static once_flag g_init_flag;
char static SliceType(int typeNum) {
    char c = 'N';
    switch(typeNum) {
        case 0:
        case 3:
        case 5:
        case 8:
            c = 'P';
            break;
        case 1:
        case 6:
            c = 'B';
            break;
        case 2:
        case 4:
        case 7:
        case 9:
            c = 'I';
            break;
        default:
            break;
    }
    return c;
}
int GetNALUS(uint8_t *data, int len) {
    int n = 0;
    int pre = 0;
    for (int i = 0; i < len; ) {
        if (data[i] == 0 &&data[i+1] == 0 ) {
            if (data[i+2] == 1 ) {
                n++;
                Log("get encoder fra nalu type %d", ((data[i+3]&0x1F)));
                if (pre != 0)
                Log("get encoder fram nalu4 size %d", (i - pre)  );
                pre = i+3;
                i += 2;
            } else if (data[i+2] == 0 &&data[i+3] ==1) {
                Log("get encoder fra nalu type %d", ((data[i+4]&0x1F)));
                if (pre != 0)
                Log("get encoder fram nalu3 size %d", (i - pre)  );
                pre = i+4;
                i+=3;
                n++;
            } else {
                i++;
            }
        } else {
            i++;
        }
    }

        Log("get encoder fram nalu size %d", len - pre  );

    return n;
}
void static checkSliceType(uint8_t *data, int len) {

    bs_t bst;
    int NRI = (data[4] >>5);
    int nalType = ((data[4]&0x1F));
    bs_init(&bst, data + 5, len - 5);
    bool firstMb = bs_read_ue(&bst);
    int sliceType = bs_read_ue(&bst);
    int ppsId =bs_read_ue(&bst);
    Log(" ===== nalType %d first mb %：：q!"
        " d sliceType %d:%c ppsId %d NRI %d",nalType, firstMb, sliceType,SliceType(sliceType), ppsId, NRI);
}
static void log_callback_null(void *ptr, int level, const char *fmt, va_list vl){
//    if (level  > AV_LOG_ERROR)
//        return;

    int cnt;
    pthread_mutex_lock(&cb_av_log_lock);
    cnt = snprintf(logBufPrefix, LOG_BUF_PREFIX_SIZE, "%s", fmt);
    cnt = vsnprintf(logBuffer, LOG_BUF_SIZE, logBufPrefix, vl);
    Log(" ffmpeg %s", logBuffer);
    pthread_mutex_unlock(&cb_av_log_lock);
}


static  void Register() {
    av_register_all();
    av_log_set_level(AV_LOG_INFO);
    av_log_set_flags(AV_LOG_SKIP_REPEATED);
    av_log_set_callback(log_callback_null);
}

Muxer::Muxer() {

}

Muxer::~Muxer() {
    if (codecContext_) {
        avcodec_close(codecContext_);
        av_free(codecContext_);
    }

}
bool Muxer::InitEncoder() {
    call_once(g_init_flag, Register);
    int ret ;
    /* find the video encoder */
    codec_ = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!codec_) {
        Log("Codec not found");
        exit(1);
    }

    codecContext_ = avcodec_alloc_context3(codec_);
    if (!codecContext_) {
        Log("Could not allocate video codec context");
        exit(1);
    }

    /* put sample parameters */
    codecContext_->bit_rate = 400000;
    /* resolution must be a multiple of two */
    codecContext_->width = 1920;
    codecContext_->height = 1080;
    /* frames per second */
    codecContext_->time_base = (AVRational){1,AV_TIME_BASE};
    codecContext_->gop_size = 10;
    codecContext_->max_b_frames = 1;
    codecContext_->pix_fmt = AV_PIX_FMT_YUV420P;

    av_opt_set(codecContext_->priv_data, "preset", "superfast", 0);
    av_opt_set_int(codecContext_->priv_data, "slice-max-size", 1000, 0);

    /* open it */
    if ((ret =avcodec_open2(codecContext_, codec_, NULL)) < 0) {
        Log("Could not open codec %d\n", ret);
        exit(1);
    }
    Log("init encoder ok");
    return true;
}
void Muxer::Encode(AVFrame* frame) {
    AVPacket pkt;
    pkt.data = NULL;    // packet data will be allocated by the encoder
    pkt.size = 0;
    int got_output = 0;
//    static int i =0;
//    frame->pts = i;
//    i++;
    int ret = avcodec_encode_video2(codecContext_, &pkt, frame, &got_output);
    if (ret < 0) {
        Log( "Error encoding frame\n");
        exit(1);
    }
    if (got_output) {
        checkSliceType(pkt.data, pkt.size);
        uint8_t *sd = av_packet_get_side_data(&pkt, AV_PKT_DATA_QUALITY_STATS,
                                              NULL);
        int pict_type = sd ? sd[4] : AV_PICTURE_TYPE_NONE;

        Log("get encoder frame  (size=%5d) slice type %d newtype %d nalus %d\n", pkt.size,
                codecContext_->coded_frame->pict_type, pict_type,
            GetNALUS(pkt.data, pkt.size));
        sleep(2);
        av_packet_unref(&pkt);
    } else {
        Log("nnnnnnn");
    }
    av_frame_unref(frame);
}