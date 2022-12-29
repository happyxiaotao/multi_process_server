#include "RtmpClient.h"
#include "../../jt1078/Jt1078Util.h"
#include "../../../core/log/Log.hpp"
#include "../../../core/ini_config.h"

std::string RtmpClient::s_rtmp_url_prefix;

RtmpClient::RtmpClient(device_id_t device_id)
    : m_device_id(device_id),
      m_bCarHaveAudio(false), m_uStartVideoTime(0), m_uLastVideoTime(0), m_pFakeAudioBuffer(nullptr), m_uMaxFakeAudioBufferLen(2048), m_uSendAudioPktCount(0)

{
    // RtmpThread的构造函数在主线程中调用，并创建。除此之外只有子线程会调用s_rtmp_url_prefix，所以不会存在多线程数据同步问题
    if (s_rtmp_url_prefix.empty())
    {
        s_rtmp_url_prefix = g_ini->Get("rtmp", "url_prefix", "");
    }

    m_pFakeAudioBuffer = new char[m_uMaxFakeAudioBufferLen];
    memset(m_pFakeAudioBuffer, '\0', m_uMaxFakeAudioBufferLen);
}

RtmpClient::~RtmpClient()
{
    if (m_pFakeAudioBuffer != nullptr)
    {
        delete[] m_pFakeAudioBuffer;
    }
}

int RtmpClient::ProcessJt1078Packet(const jt1078::packet_t &packet)
{
    if (TryToInitRtmpStream() < 0)
    {
        return -1;
    }

    // 走到这里，说明已经初始化了
    // int nSeq = packet.m_header->WdPackageSequence;
    // Trace("RtmpClient::ProcessJt1078Packet, packet seq:{}", nSeq);

    int nRet = 0;

    do
    {
        bool bComplated = false;
        // 当时原子包或者最后一个数据包时，表示读取到了全部包，进行处理
        if (packet.m_header->SubpackageHandleMark4 == RCV_ATOMIC_PACKAGE || packet.m_header->SubpackageHandleMark4 == RCV_LAST_PACKAGE)
        {
            bComplated = true;
        }
        // 是视频数据包
        if (packet.m_header->DataType4 == DATA_TYPE_VIDEO_I || packet.m_header->DataType4 == DATA_TYPE_VIDEO_P || packet.m_header->DataType4 == DATA_TYPE_VIDEO_B)
        {

            // 添加到buffer中
            m_video_buffer.append(packet.m_body, packet.m_header->WdBodyLen);

            if (bComplated)
            {
                bool bIsKeyFrame = false;

                // 这里将帧转化下
                switch (packet.m_header->DataType4)
                {
                case DATA_TYPE_VIDEO_I:
                    bIsKeyFrame = true;
                    break;
                case DATA_TYPE_VIDEO_B:
                    break;
                case DATA_TYPE_VIDEO_P:
                    break;
                default:
                    break;
                }
                nRet = ProcessVideoData(m_video_buffer.c_str(), m_video_buffer.size(), packet.m_header->Bt8timeStamp, bIsKeyFrame);
                m_video_buffer.clear(); // 使用之后，清空数据

                if (nRet < 0)
                {
                    nRet = -2;
                    break;
                }

                if (!m_bCarHaveAudio) // 设备没有上传音频数据包，需要先填充假数据，避免无音频导致的网页无法播放FLV视频
                {
                    Trace("Add Fake Audio Pcm Data,timestamp={}", packet.m_header->Bt8timeStamp);
                    AddFakeAudioPcm(packet);
                }
            }
        }
        else if (packet.m_header->DataType4 == DATA_TYPE_AUDIO)
        {
            m_bCarHaveAudio = true; // 收到音频数据包，标识设备存在音频

            m_audio_buffer.append(packet.m_body, packet.m_header->WdBodyLen);
            AUDIO_CODING_TYPE audio_type = CCodec::AUDIO_CODING_TYPE::eUnSupport;
            switch (packet.m_header->PT7)
            {
            case 6:
                audio_type = AUDIO_CODING_TYPE::eG711A;
                break;
            case 7:
                audio_type = AUDIO_CODING_TYPE::eG711U;
                break;
            case 8:
                audio_type = AUDIO_CODING_TYPE::eG726;
                break;
            case 26:
                audio_type = AUDIO_CODING_TYPE::eAdpcm;
                break;
            default:
                break;
            }
            if (audio_type == AUDIO_CODING_TYPE::eUnSupport)
            {
                uint8_t nJt1078AudioType = packet.m_header->PT7;
                Error("RtmpClient::ProcessJt1078Packet, UnSupport Jt1078 Audio Type:{}, device_id:{:14x}\n", nJt1078AudioType, m_device_id);
                nRet = -3;
                break;
            }

            if (bComplated)
            {
                DECODE_RESULT &iResult = m_audio_decoder.DecodeAudio(m_audio_buffer.c_str(), m_audio_buffer.size(), audio_type);
                m_audio_decoder.m_uPT = packet.m_header->PT7;
                int nRet = ProcessAudioData(iResult.m_pOutBuf, iResult.m_nOutBufLen, packet.m_header->Bt8timeStamp);
                m_audio_buffer.clear();
                if (nRet < 0)
                {
                    nRet = -4;
                    break;
                }
            }
        }
        else if (packet.m_header->DataType4 == DATA_TYPE_ROUTE)
        {
        }
        else
        {
            int datatype = packet.m_header->DataType4;
            Error("RtmpClient::ProcessJt1078Packet, not support 1078 data type:{}", datatype);
            nRet = -5;
            break;
        }
    } while (false);

    return nRet;
}

int RtmpClient::TryToInitRtmpStream()
{
    if (s_rtmp_url_prefix.empty())
    {
        Error("RtmpThread::TryToInitRtmpStream failed, s_rtmp_url_prefix is empty");
        return -1;
    }

    // 此RTMP流还未初始化，进行初始化操作
    if (!m_rtmp_stream.IsInit())
    {
        std::string strDeviceId = GenerateDeviceIdStrByDeviceId(m_device_id);
        m_rtmp_url = s_rtmp_url_prefix + "/" + strDeviceId;

        if (m_rtmp_stream.Init(m_rtmp_url) < 0)
        {
            Error("RtmpClient::TryToInitRtmpStream failed, rtmp_stream init failed, device_id:{:14x},rtmp_url:{}", m_device_id, m_rtmp_url);
            return -2;
        }
        Info("RtmpClient::TryToInitRtmpStream, rmpt_stream init succ, rtmp_url:{}", m_rtmp_url);
    }

    return 0;
}

int RtmpClient::ProcessVideoData(const char *buffer, size_t len, uint64_t timestamp, bool bKeyFrame)
{
    // Trace("RtmpClient, Write Video Data, timestamp:{}", timestamp);
    return m_rtmp_stream.WriteData(AVMEDIA_TYPE_VIDEO, (char *)buffer, len, bKeyFrame, timestamp);
}

int RtmpClient::ProcessAudioData(const char *buffer, size_t len, uint64_t timestamp)
{
    // Trace("RtmpClient, Write Audio Data, timestamp:{}", timestamp);
    return m_rtmp_stream.WriteData(AVMEDIA_TYPE_AUDIO, (char *)buffer, len, false, timestamp);
}

int RtmpClient::AddFakeAudioPcm(const jt1078::packet_t &packet)
{                                 // 视频是25帧，一帧40ms。所以音频也要填充40ms的音频数据
    uint64_t uAudioDataLen = 640; // 采样率8kHz,单通道，8bit位。640个字节对应80ms。实际当做40ms进行测试
    uint64_t timestamp = packet.m_header->Bt8timeStamp;

    if (m_uStartVideoTime != 0 && m_uLastVideoTime != 0 && timestamp > m_uLastVideoTime)
    {
        uint64_t uDiff = timestamp - m_uStartVideoTime;
        size_t nCount = (uDiff + 39) / 40;
        if (nCount > m_uSendAudioPktCount)
        {
            uAudioDataLen = 640 * (nCount - m_uSendAudioPktCount); // 发送对应数据的音频包
            uAudioDataLen = std::min(m_uMaxFakeAudioBufferLen, uAudioDataLen);
            m_uSendAudioPktCount = nCount;
        }
        else
        {
            uAudioDataLen = 0; // 不发送
        }
    }
    int nRet = 0;
    // m_pFakeAudioBuffer内容全是'\0'，直接写入对应长度即可
    if (uAudioDataLen > 0)
    {
        nRet = ProcessAudioData(m_pFakeAudioBuffer, uAudioDataLen, timestamp);
    }

    if (m_uStartVideoTime == 0)
    {
        m_uStartVideoTime = timestamp;
    }
    m_uLastVideoTime = timestamp;

    return nRet;
}
