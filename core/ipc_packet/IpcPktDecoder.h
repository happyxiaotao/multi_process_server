#ifndef IPC_PKT_DECODER_H
#define IPC_PKT_DECODER_H
#include "IpcPacket.h"
#include "../buffer/Buffer.h"
namespace ipc
{
    class decoder_t
    {
        enum ReadStatus : int
        {
            kReadHeader = 0, // 第一步解析头
            kReadBody,       // 第二步解析包体
        };

    public:
        enum ErrorType : int
        {
            kNoError = 0,
            kNeedMoreData,  //需要更多的数据，当前数据不够
            kBufferFull,    // m_buffer的缓存区空间不够
            kInvalidHeader, // 非法的包头
        };

        const size_t MAX_BUFFER_SIZE = 500 * 1024; // m_buffer的最大缓存大小

    public:
        decoder_t() : m_howmuch(sizeof(packet_t)),
                      m_read_status(kReadHeader)
        {
            m_buffer.Reserve(MAX_BUFFER_SIZE / 4);
            m_packet_buffer.reserve(1024);
        }
        ~decoder_t()
        {
        }

    public:
        ErrorType PushBuffer(const char *data, size_t len);
        ErrorType PushBuffer(const Buffer &buffer);
        ErrorType Decode(); // 一次只解析一个数据包，需要循环调用此函数

        inline const packet_t &GetPacket() const { return *(packet_t *)(m_packet_buffer.c_str()); }

    private:
        inline packet_t &_GetPacket() { return *(packet_t *)(m_packet_buffer.c_str()); }

    private:
        ErrorType ParseHeader();
        ErrorType ParseBody();

    private:
        Buffer m_buffer;
        std::string m_packet_buffer; //存放的有packet数据，需要通过指针使用（当做一个char[]数组使用，自带扩容机制）
        size_t m_howmuch;            // 需要读取的数据长度，要求最多长度不超过
        ReadStatus m_read_status;
    };
} // namespace  ipc

#endif // IPC_PKT_DECODER_H