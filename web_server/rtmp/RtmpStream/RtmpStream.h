//
// Created by hc on 2020/5/14.
// Modify by sjt on 2022/12/28
//

#ifndef C20_CRTMPSTREAM_H
#define C20_CRTMPSTREAM_H

#include "./FFMPEG.h"
#include <string>
typedef struct OutputStream
{
    AVStream *st;
    AVCodecContext *enc;

    int64_t next_pts;
    long long samples_count;

    AVFrame *frame;
    AVFrame *tmp_frame;

    struct SwrContext *swr_ctx;
} OutputStream;

class CRtmpStream
{
public:
    CRtmpStream();
    virtual ~CRtmpStream();

    // 是否连接成功
    bool IsInit() { return m_bConnectRtmpSucc; }

    int Init(const char *filename);
    int Init(const std::string &filename) { return Init(filename.c_str()); }
    int WriteData(AVMediaType datatype, char *data, int datalen, bool bIsKeyFrame, uint64_t timestamp);
    bool GetPushState() const;
    std::string GetUrl() const;

private:
    void SetPushState(bool bIsPush);
    int write_frame(AVFormatContext *fmt_ctx, const AVRational *time_base, AVStream *st, AVPacket *pkt);

    int add_stream(OutputStream *ost, AVCodec **codec, enum AVCodecID codec_id);

    int open_audio(AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg);
    int write_audio_frame(OutputStream *ost, char *data, int datalen, uint64_t timestamp);

    int open_video(AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg);
    int write_video_frame(OutputStream *ost, char *data, int datalen, bool bIsKeyFrame, uint64_t timestamp);

    void close_stream(OutputStream *ost);

    bool isIdrFrame2(uint8_t *buf, int len);
    bool isIdrFrame1(uint8_t *buf, int size);

private:
    int32_t GetAVPts(uint64_t &ts_last, uint32_t &offset, uint64_t timestamp);

private:
    OutputStream video_st, audio_st;
    AVOutputFormat *fmt;
    AVFormatContext *oc;
    AVCodec *audio_codec, *video_codec;
    AVDictionary *opt;

    int have_video, have_audio;
    int ptsInc;

    int iRawLineSize;
    int iRawBuffSize;
    uint8_t *pRawBuff;

    int iConvertLineSize;
    int iConvertBuffSize;
    uint8_t *pConvertBuff;

    char pcmencodebuf[4096];
    int pcmencodesize;
    bool m_bIsPushing;
    std::string m_sUrl;

    uint64_t m_ts_audio_last = 0;
    uint32_t m_ts_audio_offset = 0;
    uint64_t m_ts_video_last = 0;
    uint32_t m_ts_video_offset = 0;

    bool m_bConnectRtmpSucc; // 连接rtmp服务器是否成功
};

#endif // C20_CRTMPSTREAM_H
