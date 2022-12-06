#ifndef IPC_PACKET_H
#define IPC_PACKET_H
#include "IpcPktType.h"
namespace ipc
{
  const uint32_t INVALID_PKT_SEQ_ID = -1;
#pragma pack(1)
  struct packet_t
  {
    packet_t() : m_uHeadLength(sizeof(packet_t)), m_uDataLength(0), m_uPktSeqId(INVALID_PKT_SEQ_ID), m_uPktType(IPC_PKT_INVALID_PACKET)
    {
    }
    void Clear()
    {
      m_uHeadLength = sizeof(packet_t);
      m_uDataLength = 0;
      m_uPktSeqId = INVALID_PKT_SEQ_ID;
      m_uPktType = IPC_PKT_INVALID_PACKET;
    }
    uint32_t m_uHeadLength; // 包头长度
    uint32_t m_uDataLength; // 数据体长度
    uint32_t m_uPktSeqId;   // 序号id
    uint32_t m_uPktType;    // 数据包类型
    char m_data[0];         // 使用变长数组，指向后面的数据内容
  };
#pragma pack()
} // namespace ipc

#endif // IPC_PACKET_H