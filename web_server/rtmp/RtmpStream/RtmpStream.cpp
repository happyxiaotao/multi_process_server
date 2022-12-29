//
// Created by hc on 2020/5/14.
// Modify by sjt on 2022/12/28
//

#include "RtmpStream.h"
#include "../../../core/log/Log.hpp"

CRtmpStream::CRtmpStream()
    : m_bIsPushing(false)
{
    have_video = 0;
    have_audio = 0;
    ptsInc = 0;

    fmt = NULL;
    oc = NULL;
    opt = NULL;
    video_st = {0};
    audio_st = {0};

    audio_codec = NULL;
    video_codec = NULL;

    iRawLineSize = 0;
    iRawBuffSize = 0;
    pRawBuff = NULL;

    iConvertLineSize = 0;
    iConvertBuffSize = 0;
    pConvertBuff = NULL;

    memset(pcmencodebuf, 0, 4096);
    pcmencodesize = 0;

    m_bConnectRtmpSucc = false;
}

CRtmpStream::~CRtmpStream()
{
    /* Write the stream trailer to an output media file */
    if (oc && m_bConnectRtmpSucc)
    {
        av_write_trailer(oc); // sjt note: 编写结束信息
    }

    /* Close each codec. */
    if (have_video)
        close_stream(&video_st);
    if (have_audio)
        close_stream(&audio_st);

    /* Close the output file. */
    if (fmt)
    {
        if (!(fmt->flags & AVFMT_NOFILE))
            avio_closep(&oc->pb);
    }

    /* free the stream */
    if (oc)
        avformat_free_context(oc);

    /* free the audio frame */
    if (pRawBuff)
        av_free(pRawBuff);
    if (pConvertBuff)
        av_free(pConvertBuff);
    if (m_bIsPushing)
    {
        SetPushState(false);
    }
}

int CRtmpStream::Init(const char *filename)
{
    int ret = 0;
    if (filename == nullptr)
    {
        return -1;
    }
    m_sUrl = filename;

    av_register_all();
    avformat_network_init();

    /* allocate the output media context */
    avformat_alloc_output_context2(&oc, NULL, "flv", filename); // sjt note:创建AVFormatContext，格式为flv
    if (!oc)
    {
        Error("CRtmpStream::Init failed,Could not deduce output format from file extension.");
        return -1;
    }

    fmt = oc->oformat;

    /* Add the audio and video streams using the default format codecs
     * and initialize the codecs. */
    if (fmt->video_codec != AV_CODEC_ID_NONE)
    {
        fmt->video_codec = AV_CODEC_ID_H264;
        ret = add_stream(&video_st, &video_codec, fmt->video_codec);
        if (ret < 0)
        {
            Error("CRtmpStream::Init failed,Could not add video stream.");
            return -1;
        }
        have_video = 1;
    }
    if (fmt->audio_codec != AV_CODEC_ID_NONE)
    {
        fmt->audio_codec = AV_CODEC_ID_AAC;
        ret = add_stream(&audio_st, &audio_codec, fmt->audio_codec);
        if (ret < 0)
        {
            Error("CRtmpStream::Init failed,Could not add audio stream.");
            return -1;
        }
        have_audio = 1;
    }

    /* Now that all the parameters are set, we can open the audio and
     * video codecs and allocate the necessary encode buffers. */
    if (have_video)
    {
        ret = open_video(video_codec, &video_st, opt);
        if (ret < 0)
        {
            Error("CRtmpStream::Init failed,Could not open video.");
            return -1;
        }
    }

    if (have_audio)
    {
        ret = open_audio(audio_codec, &audio_st, opt);
        if (ret < 0)
        {
            Error("CRtmpStream::Init failed,Could not open audio.");
            return -1;
        }
    }

    av_dump_format(oc, 0, filename, 1);

    /* open the output file, if needed */
    if (!(fmt->flags & AVFMT_NOFILE))
    {
        ret = avio_open(&oc->pb, filename, AVIO_FLAG_WRITE); // sjt note:不能设置SRS的http连接回调on_connect，否则会导致此处阻塞
        if (ret < 0)
        {
            Error("CRtmpStream::Init failed, Could not open {}", filename);
            return -1;
        }
    }

    /* Write the stream header, if any. */
    m_bConnectRtmpSucc = true;
    ret = avformat_write_header(oc, &opt);
    if (ret < 0)
    {
        Error("CRtmpStream::Init failed,Error occurred when opening output file");
        return -1;
    }
    m_bIsPushing = true;
    return 0;
}
/*
 * If the data is video, the input is H264;
 * If the data is audio, the input is PCM;
 */
int CRtmpStream::WriteData(AVMediaType datatype, char *data, int datalen, bool bIsKeyFrame, uint64_t timestamp)
{
    int ret = 0;

    if (AVMEDIA_TYPE_VIDEO == datatype)
    {
        ret = write_video_frame(&video_st, data, datalen, bIsKeyFrame, timestamp);
    }
    else if (AVMEDIA_TYPE_AUDIO == datatype)
    {
        ret = write_audio_frame(&audio_st, data, datalen, timestamp);
    }

    return ret;
}

int CRtmpStream::write_frame(AVFormatContext *fmt_ctx, const AVRational *time_base, AVStream *st, AVPacket *pkt)
{
    /* rescale output packet timestamp values from codec to stream timebase */
    // 目前暂时不需要设置这么多时间，手动设置。 av_packet_rescale_ts(pkt, *time_base, st->time_base);
    pkt->stream_index = st->index;

    // printf("\t\t\tpts=%lld, dts=%lld, duration=%lld, flags=%d, \n", pkt->pts, pkt->dts, pkt->duration, pkt->flags);

    /* Write the compressed frame to the media file. */
    return av_interleaved_write_frame(fmt_ctx, pkt);
}

/* Add an output stream. */
int CRtmpStream::add_stream(OutputStream *ost, AVCodec **codec, enum AVCodecID codec_id)
{
    AVCodecContext *c = NULL;
    AVRational a_time_base;
    AVRational v_time_base;

    /* find the encoder */
    *codec = avcodec_find_encoder(codec_id); // sjt note:可以根据AVCodecID，来找到对应的编码器。
    if (!(*codec))
    {
        Error("CRtmpStream::add_stream failed, Could not find encoder for {}", avcodec_get_name(codec_id));
        return -1;
    }

    ost->st = avformat_new_stream(oc, NULL); // sjt note: 创建心的stream数据
    if (!ost->st)
    {
        Error("CRtmpStream::add_stream failed, Could not allocate stream");
        return -1;
    }
    ost->st->id = oc->nb_streams - 1;

    c = avcodec_alloc_context3(*codec);
    if (!c)
    {
        Error("CRtmpStream::add_stream failed, Could not alloc an encoding context");
        return -1;
    }
    ost->enc = c;

    switch ((*codec)->type)
    {
    case AVMEDIA_TYPE_AUDIO:
        c->codec_id = codec_id;
        c->codec_type = AVMEDIA_TYPE_AUDIO;
        /**
         * 对于浮点格式，其值在[-1.0,1.0]之间，任何在该区间之外的值都超过了最大音量的范围。
            和YUV的图像格式格式，音频的采样格式分为平面（planar）和打包（packed）两种类型，
            在枚举值中上半部分是packed类型，后面（有P后缀的）是planar类型。
         */
        c->sample_fmt = AV_SAMPLE_FMT_FLTP; // 音频采样格式
        // c->bit_rate = 64000;
        c->bit_rate = 48000;
        c->sample_rate = 8000;
        // c->channel_layout = AV_CH_LAYOUT_MONO; //声道数
        c->channel_layout = 1; // 声道数
        c->channels = av_get_channel_layout_nb_channels(c->channel_layout);
        a_time_base = {1, c->sample_rate};
        ost->st->time_base = a_time_base;
        break;

    case AVMEDIA_TYPE_VIDEO:
        c->codec_id = codec_id;
        c->codec_type = AVMEDIA_TYPE_VIDEO;
        c->pix_fmt = AV_PIX_FMT_YUV420P;
        // c->bit_rate = 400000;
        c->width = 352;
        c->height = 288;
        v_time_base = {1, 25};
        ost->st->time_base = v_time_base;
        c->time_base = ost->st->time_base;
        c->codec_tag = 0;

        // c->rc_buffer_size = 20000;

        break;

    default:
        break;
    }

    /* Some formats want stream headers to be separate. */
    if (oc->oformat->flags & AVFMT_GLOBALHEADER)
        c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    return 0;
}

int CRtmpStream::open_audio(AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg)
{
    int ret = 0;
    int nb_samples = 0;
    AVDictionary *opt = NULL;
    AVCodecContext *c = ost->enc;

    /* open it */
    av_dict_copy(&opt, opt_arg, 0);
    ret = avcodec_open2(c, codec, &opt);
    av_dict_free(&opt);
    if (ret < 0)
    {
        Error("CRtmpStream::open_audio failed, Could not open audio codec");
        return -1;
    }

    /* copy the stream parameters to the muxer */
    ret = avcodec_parameters_from_context(ost->st->codecpar, c);
    if (ret < 0)
    {
        Error("CRtmpStream::open_audio failed, Could not copy the stream parameters");
        return -1;
    }

    nb_samples = c->frame_size;

    /* init and alloc input(pcm) audio frame */
    iRawBuffSize = av_samples_get_buffer_size(&iRawLineSize, c->channels, nb_samples, AV_SAMPLE_FMT_S16, 0);
    pRawBuff = (uint8_t *)av_malloc(iRawBuffSize);

    ost->tmp_frame = av_frame_alloc();
    ost->tmp_frame->nb_samples = nb_samples;
    ost->tmp_frame->format = AV_SAMPLE_FMT_S16;
    ost->tmp_frame->channels = c->channels;

    ret = avcodec_fill_audio_frame(ost->tmp_frame, c->channels, AV_SAMPLE_FMT_S16, (const uint8_t *)pRawBuff, iRawBuffSize, 0);
    if (ret < 0)
    {
        Error("CRtmpStream::open_audio failed, Could not fill input audio frame");
        return -1;
    }

    /* init and alloc need resample(aac) audio frame */
    iConvertBuffSize = av_samples_get_buffer_size(&iConvertLineSize, c->channels, nb_samples, c->sample_fmt, 0);
    pConvertBuff = (uint8_t *)av_malloc(iConvertBuffSize);

    ost->frame = av_frame_alloc();
    ost->frame->nb_samples = nb_samples;
    ost->frame->format = c->sample_fmt;
    ost->frame->channels = c->channels;

    ret = avcodec_fill_audio_frame(ost->frame, c->channels, c->sample_fmt, (const uint8_t *)pConvertBuff, iConvertBuffSize, 0);
    if (ret < 0)
    {
        Error("CRtmpStream::open_audio failed, Could not fill resample audio frame");
        return -1;
    }

    /* create resampler context */
    ost->swr_ctx = swr_alloc();
    if (!ost->swr_ctx)
    {
        Error("CRtmpStream::open_audio failed, Could not allocate resampler context");
        return -1;
    }

    /* set options */
    av_opt_set_int(ost->swr_ctx, "in_channel_count", c->channels, 0);
    av_opt_set_int(ost->swr_ctx, "in_sample_rate", c->sample_rate, 0);
    av_opt_set_sample_fmt(ost->swr_ctx, "in_sample_fmt", AV_SAMPLE_FMT_S16, 0);
    av_opt_set_int(ost->swr_ctx, "out_channel_count", c->channels, 0);
    av_opt_set_int(ost->swr_ctx, "out_sample_rate", c->sample_rate, 0);
    av_opt_set_sample_fmt(ost->swr_ctx, "out_sample_fmt", c->sample_fmt, 0);

    /* initialize the resampling context */
    if ((ret = swr_init(ost->swr_ctx)) < 0)
    {
        Error("CRtmpStream::open_audio failed, Failed to initialize the resampling context");
        return -1;
    }

    return 0;
}

/*
 * encode one audio frame and send it to the muxer
 * return 1 when encoding is finished, 0 otherwise
 */
int CRtmpStream::write_audio_frame(OutputStream *ost, char *data, int datalen, uint64_t timestamp)
{
    // printf("write_audio_frame, datalen=%d, pcmencodesize=%d, iRawBuffSize=%d\n", datalen, pcmencodesize, iRawBuffSize);
    int ret = 0;
    int got_packet = 0;
    int dst_nb_samples = 0;
    AVCodecContext *c = ost->enc;
    AVPacket pkt = {0};
    AVFrame *frame = NULL;

    av_init_packet(&pkt);

    frame = ost->tmp_frame;

    /* Compose a frame of data to be resampled */
    memcpy(&pcmencodebuf[pcmencodesize], data, datalen);
    pcmencodesize += datalen;
    if (pcmencodesize >= iRawBuffSize)
    {
        memcpy(pRawBuff, pcmencodebuf, iRawBuffSize);

        pcmencodesize -= iRawBuffSize;
        memcpy(&pcmencodebuf[0], &pcmencodebuf[iRawBuffSize], pcmencodesize);

        frame->pts = ost->next_pts;
        ost->next_pts += frame->nb_samples;
    }
    else
    {
        return 0;
    }

    if (frame)
    {
        dst_nb_samples = frame->nb_samples;

        ret = swr_convert(ost->swr_ctx, (uint8_t **)ost->frame->data, frame->nb_samples, (const uint8_t **)frame->data, frame->nb_samples);
        if (ret < 0)
        {
            Error("CRtmpStream::open_audio_frame failed, Error while converting");
            return -1;
        }

        frame = ost->frame;
        frame->pts = ost->samples_count;
        ost->samples_count += dst_nb_samples;
    }

    ret = avcodec_encode_audio2(c, &pkt, frame, &got_packet);
    if (ret < 0)
    {
        Error("CRtmpStream::open_audio_frame failed, Error encoding audio frame");
        return -1;
    }

    if (got_packet)
    {
        // 直接设置为真实的时间戳
        uint64_t last_ts = m_ts_audio_last;
        // printf("write audio data, timestamp=%llu, last timestamp:%llu\n", timestamp, last_ts);
        int32_t ts = GetAVPts(m_ts_audio_last, m_ts_audio_offset, timestamp);

        pkt.pts = ts;
        pkt.dts = ts;

        if (last_ts > timestamp) // 跳过不处理
        {
            return 0;
        }
        // pkt.duration = (last_ts < timestamp) ? (timestamp - last_ts) : 0;
        pkt.duration = timestamp - last_ts;

        // printf("write audio data, timestamp=%llu\n", timestamp);
        ret = write_frame(oc, &c->time_base, ost->st, &pkt);
        if (ret < 0)
        {
            Error("CRtmpStream::open_audio_frame failed, Error while writing audio frame, url:{}", m_sUrl);
            return -1;
        }
    }

    return 0;
}

int CRtmpStream::open_video(AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg)
{
    int ret = 0;
    AVDictionary *opt = NULL;
    AVCodecContext *c = ost->enc;

    av_dict_copy(&opt, opt_arg, 0);
    /* open the codec */
    ret = avcodec_open2(c, codec, &opt);
    av_dict_free(&opt);
    if (ret < 0)
    {
        Error("CRtmpStream::open_video failed, Could not open video codec");
        return -1;
    }

    /* copy the stream parameters to the muxer */
    ret = avcodec_parameters_from_context(ost->st->codecpar, c);
    if (ret < 0)
    {
        Error("CRtmpStream::open_video failed, Could not copy the stream parameters");
        return -1;
    }

    // Trace("CRtmpStream::open_video, succ open video codec");
    return 0;
}

/*
 * encode one video frame and send it to the muxer
 * return 1 when encoding is finished, 0 otherwise
 */
int CRtmpStream::write_video_frame(OutputStream *ost, char *data, int datalen, bool bIsKeyFrame, uint64_t timestamp)
{
    int ret = 0;
    int isI = 0;
    AVCodecContext *c = ost->enc;
    AVPacket pkt = {0};

    av_init_packet(&pkt);

    isI = isIdrFrame1((uint8_t *)data, datalen);
    pkt.flags |= isI ? AV_PKT_FLAG_KEY : 0;
    pkt.data = (uint8_t *)data;
    pkt.size = datalen;

    AVRational time_base = {1, 1000};                                      // sjt note: 作为时间基，将一秒分为1000份。每个单位1/1000秒
    pkt.pts = av_rescale_q((ptsInc++) * 2, time_base, ost->st->time_base); // sjt note:这里计算出来的pts是播放时间，单位是秒。
    // sjt note： 这里pts表示图形展示时间，ptsInc应该递增，来表示每一帧。
    pkt.dts = av_rescale_q_rnd(pkt.dts, ost->st->time_base, ost->st->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
    pkt.duration = av_rescale_q(pkt.duration, ost->st->time_base, ost->st->time_base);
    pkt.pos = -1;

    // 直接设置为真实的时间戳
    uint64_t last_ts = m_ts_video_last;
    // printf("write video data, timestamp=%llu\n", timestamp);
    int32_t ts = GetAVPts(m_ts_video_last, m_ts_video_offset, timestamp);

    pkt.pts = ts;
    pkt.dts = ts;

    if (last_ts > timestamp)
    {
        return 0;
    }
    pkt.duration = (last_ts < timestamp) ? (timestamp - last_ts) : 0;

    ret = write_frame(oc, &c->time_base, ost->st, &pkt);
    if (ret < 0)
    {
        Error("CRtmpStream::write_video_frame, Error while writing video frame, url:{}", m_sUrl);
        return -1;
    }

    return 0;
}

void CRtmpStream::close_stream(OutputStream *ost)
{
    avcodec_free_context(&ost->enc);
    av_frame_free(&ost->frame);
    av_frame_free(&ost->tmp_frame);
    swr_free(&ost->swr_ctx);
}

bool CRtmpStream::isIdrFrame2(uint8_t *buf, int len)
{
    switch (buf[0] & 0x1f)
    {
    case 7: // SPS
        return true;
    case 8: // PPS
        return true;
    case 5:
        return true;
    case 1:
        return false;

    default:
        return false;
        break;
    }
    return false;
}

bool CRtmpStream::isIdrFrame1(uint8_t *buf, int size)
{
    int last = 0;
    for (int i = 2; i <= size; ++i)
    {
        if (i == size)
        {
            if (last)
            {
                bool ret = isIdrFrame2(buf + last, i - last);
                if (ret)
                {
                    return true;
                }
            }
        }
        else if (buf[i - 2] == 0x00 && buf[i - 1] == 0x00 && buf[i] == 0x01)
        {
            if (last)
            {
                int size = i - last - 3;
                if (buf[i - 3])
                    ++size;
                bool ret = isIdrFrame2(buf + last, size);
                if (ret)
                {
                    return true;
                }
            }
            last = i + 1;
        }
    }
    return false;
}

void CRtmpStream::SetPushState(bool bIsPush)
{
    m_bIsPushing = bIsPush;
}

bool CRtmpStream::GetPushState() const
{
    return m_bIsPushing;
}

std::string CRtmpStream::GetUrl() const
{
    return m_sUrl;
}

int32_t CRtmpStream::GetAVPts(uint64_t &ts_last, uint32_t &uOffset, uint64_t timestamp)
{
    if (ts_last == 0)
    {
        ts_last = timestamp;
        uOffset = 0; // 此时uOffset也应该为0
    }
    else
    {
        if (ts_last > timestamp)
        {
            ; //
        }
        else
        {
            int32_t diff = timestamp - ts_last;
            uOffset += diff;
            ts_last = timestamp;
        }
    }
    return uOffset;
}