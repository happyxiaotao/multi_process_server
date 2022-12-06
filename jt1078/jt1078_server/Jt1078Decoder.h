#ifndef JT_1078_SERVER_DECODER_H
#define JT_1078_SERVER_DECODER_H

#include "../../core/buffer/Buffer.h"
#include "Jt1078Packet.h"

namespace jt1078
{
    class decoder_t
    {
        size_t MAX_BUFFER_SIZE = 500 * 1024;           // m_buffer读取缓存区的最大容量
        size_t READ_SIZE_FOR_HEADER_FIRST = 16;        // 最开始应该读取的字节个数，是16个，应为不同的类型，包头大小还不一样
        size_t READ_MIN_SIZE_FOR_HEADER_SECOND = 2;    // 步骤2最少应该有的数据个数(透传数据的情况)，实际会比这个多
        size_t READ_VIDEO_SIZE_FOR_HEADER_SECOND = 14; // 视频包，包头剩余大小
        size_t READ_AUDIO_SIZE_FOR_HEADER_SECOND = 10; // 音频包，包头剩余大小
        enum ReadStatus : int
        {
            kReadHeaderStep1 = 0, // 第一步解析头16个字节
            kReadHeaderStep2,     // 第二步解析头的剩余字节
            kReadBody,            // 第三步，解析包体内容
        };

    public:
        enum ErrorType : int
        {
            kNoError = 0,
            kNeedMoreData,      //需要更多的数据，当前数据不够
            kBufferFull,        // 缓存区已满
            kInvalidHeader,     // 非法的包头
            kBodyLengthInvalid, //包头中数据体长度超过最大长度
        };

    public:
        decoder_t();
        ~decoder_t() {}

    public:
        ErrorType PushBuffer(const Buffer &buffer);
        ErrorType Decode(); // 一次只解析一个数据包，需要循环调用此函数

        inline const packet_t &GetPacket() const { return m_packet; }
        inline size_t GetReadableBytes() { return m_buffer.ReadableBytes(); }
        inline size_t GetHowmuch() { return m_howmuch; }

    private:
        ErrorType ParseHeaderStep1(header_t *header);
        ErrorType ParseHeaderStep2(header_t *header);
        ErrorType ParseBody(packet_t *packet);

        bool IsSupportCodingType(size_t uCodingType);

    private:
        Buffer m_buffer;
        packet_t m_packet;
        size_t m_howmuch; // 需要读取的数据长度，要求最多长度不超过
        ReadStatus m_read_status;
    };
} // namespace jt1078

#endif // JT_1078_SERVER_DECODER_H