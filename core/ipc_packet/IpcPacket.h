#ifndef IPC_PACKET_H
#define IPC_PACKET_H
#include <string.h>
#include "IpcPktType.h"
#include <utility> //for std::move
namespace ipc
{
  const uint32_t INVALID_PKT_SEQ_ID = -1;
#pragma pack(1)
  struct packet_t
  {
    packet_t()
    {
      Clear();
    }

    packet_t(const packet_t &other) = delete;
    void operator=(const packet_t &other) = delete;

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
    char m_data[0];         // 使用柔性数组，自动指向后面的数据内容
  };
#pragma pack()

#pragma pack(1)
  // 支持移动构造的结构体，避免柔性数组无法进行移动构造的情况
  struct packet_move_t
  {
    packet_move_t()
        : m_uHeadLength(sizeof(packet_move_t)), m_uDataLength(0), m_uPktSeqId(INVALID_PKT_SEQ_ID), m_uPktType(IPC_PKT_INVALID_PACKET), m_data(nullptr)
    {
    }
    packet_move_t(const packet_move_t &other)
    {
      CopyFrom(other);
    }
    packet_move_t(packet_move_t &&other)
    {
      MoveFrom(std::move(other));
    }
    void operator=(const packet_move_t &other)
    {
      Clear();
      CopyFrom(other);
    }
    void operator=(packet_move_t &&other)
    {
      Clear();
      MoveFrom(std::move(other));
    }

    // 从packet_t拷贝数据
    packet_move_t(const packet_t &other)
    {
      CopyFrom(other);
    }
    // 从packet_t拷贝数据
    void operator=(const packet_t &other)
    {
      Clear();
      CopyFrom(other);
    }

    void CopyFrom(const packet_move_t &other)
    {
      m_uHeadLength = other.m_uHeadLength;
      m_uDataLength = other.m_uDataLength;
      m_uPktSeqId = other.m_uPktSeqId;
      m_uPktType = other.m_uPktType;
      if (other.m_data != nullptr)
      {
        m_data = new char[other.m_uDataLength + 1];
        memcpy(m_data, other.m_data, other.m_uDataLength);
        m_data[other.m_uDataLength] = '\0';
      }
    }

    void MoveFrom(packet_move_t &&other)
    {
      memcpy(this, &other, sizeof(other));
      other.m_data = nullptr;
      other.Clear();
    }

    // 注意：是从packet_t拷贝数据，而不是packet_move_t，不能直接将所有数据进行赋值
    void CopyFrom(const packet_t &other)
    {
      m_uHeadLength = sizeof(packet_move_t);
      m_uDataLength = other.m_uDataLength;
      m_uPktSeqId = other.m_uPktSeqId;
      m_uPktType = other.m_uPktType;
      if (other.m_uDataLength > 0)
      {
        m_data = new char[other.m_uDataLength + 1];
        memcpy(m_data, other.m_data, other.m_uDataLength);
        m_data[other.m_uDataLength] = '\0';
      }
    }

    void Clear()
    {
      m_uHeadLength = sizeof(packet_move_t);
      m_uDataLength = 0;
      m_uPktSeqId = INVALID_PKT_SEQ_ID;
      m_uPktType = IPC_PKT_INVALID_PACKET;
      if (m_data != nullptr)
      {
        delete[] m_data;
        m_data = nullptr;
      }
    }
    uint32_t m_uHeadLength; // 包头长度
    uint32_t m_uDataLength; // 数据体长度
    uint32_t m_uPktSeqId;   // 序号id
    uint32_t m_uPktType;    // 数据包类型
    char *m_data;           // 使用指针代替m_data[0]。来支持移动构造
  };
#pragma pack()

} // namespace ipc

#endif // IPC_PACKET_H