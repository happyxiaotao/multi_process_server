#ifndef PIPE_PACKET_H
#define PIPE_PACKET_H
#include "PipePktType.h"

// 下面的 SPipePacket用来实现两个线程之间的数据交互
// 不能多个线程同时往一个管道写数据，因为无法保证原子性
#pragma pack(1)
struct PipePacket
{
    PipePacket() : m_uHeadLength(sizeof(PipePacket)), m_uDataLength(0), m_uPktType(PIPE_PKT_TYPE_INVALID)
    {
    }
    uint32_t m_uHeadLength; // 包头长度
    uint32_t m_uDataLength; // 数据体长度
    uint32_t m_uPktType;    // 数据包类型
    char m_data[0];         // 使用变长数组，指向后面的数据内容
};
#pragma pack()

#endif // PIPE_PACKET_H