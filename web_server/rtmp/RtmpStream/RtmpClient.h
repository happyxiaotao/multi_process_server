#ifndef RTMP_CLIENT_H
#define RTMP_CLIENT_H

#include "RtmpStream.h"
#include "../../../jt1078/AV_Common_Define.h"
#include "../../../jt1078/jt1078_server/Jt1078Packet.h"
#include "../../audio/CCodec.h"

class RtmpClient
{
public:
    RtmpClient(device_id_t device_id, const std::string &rtmp_url_prefix);
    ~RtmpClient();

public:
    int ProcessJt1078Packet(const jt1078::packet_t &packet);

private:
    int TryToInitRtmpStream();
    int ProcessVideoData(const char *buffer, size_t len, uint64_t timestamp, bool bKeyFrame);
    int ProcessAudioData(const char *buffer, size_t len, uint64_t timestamp);

    int AddFakeAudioPcm(const jt1078::packet_t &packet);

private:
    device_id_t m_device_id;       // device_id
    std::string m_rtmp_url_prefix; // rtmp地址前缀
    std::string m_rtmp_url;

    CRtmpStream m_rtmp_stream; // rtmp流

    std::string m_video_buffer; // 视频数据包
    std::string m_audio_buffer; // 音频数据包

    CCodec m_audio_decoder; // 负责音频解码，解码为PCM格式

    // 下面的几个变量，是用来上传空的音频数据的
    bool m_bCarHaveAudio;            // 有些汽车不会上传音频数据，导致合成的FLV，网页无法播放。这里传递假音频数据进行处理
    uint64_t m_uStartVideoTime;      // 收到的第一个视频包的时间
    uint64_t m_uLastVideoTime;       // 当上传假音频包时，保存的之前视频帧时间（根据视频时间，进行填充）
    char *m_pFakeAudioBuffer;        // 假的音频数据包缓存
    size_t m_uMaxFakeAudioBufferLen; // 传假包使用
    size_t m_uSendAudioPktCount;     // 传假包使用
};

#endif // RTMP_CLIENT_H