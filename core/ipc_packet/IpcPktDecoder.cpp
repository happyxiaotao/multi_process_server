#include "IpcPktDecoder.h"
#include "../log/Log.hpp"

namespace ipc
{
    decoder_t::ErrorType decoder_t::PushBuffer(const Buffer &buffer)
    {
        return PushBuffer(buffer.GetBuffer(), buffer.ReadableBytes());
    }
    decoder_t::ErrorType decoder_t::PushBuffer(const char *buffer, size_t len)
    {
        // 将数据拷贝到m_buffer中
        size_t nCapacity = m_buffer.GetCapacity();
        size_t nLeftCapacity = m_buffer.GetLeftCapacity();
        size_t nNewBufferLen = len;
        size_t nCurBufferLen = m_buffer.ReadableBytes();

        // 目前预分配的空间不够，即使是最大空间，也不够。需要再次开辟，这里报错处理
        if (nNewBufferLen + nCurBufferLen > MAX_BUFFER_SIZE)
        {
            Error("ipc::decoder_t::PushBuffer,error=kBufferFull,nNewBufferLen:{},MAX_BUFFER_SIZE:{},nCurBufferLen:{},nCapacity:{}",
                  nNewBufferLen, MAX_BUFFER_SIZE, nCurBufferLen, nCapacity);
            return kBufferFull;
        }
        // 剩余要求的空间没有达到最大要求，但是也需要开辟空间来存放数据，新开辟的空间足够存储
        else if (nNewBufferLen + nCurBufferLen > nCapacity)
        {
            size_t nNewCapacity = std::min<size_t>(nCapacity * 2, MAX_BUFFER_SIZE);
            //   Warn("decoder_t::PushBuffer,need alloc memroy,nNewBufferLen:{},nCurBufferLen:{},nCapacity:{},newCapacity:{}",
            //        nNewBufferLen, nCurBufferLen, nCapacity, nNewCapacity);
            m_buffer.AutoRemove();
            m_buffer.Reserve(nNewCapacity);
            m_buffer.Append(buffer, len);
        }
        // 新加的数据，只需要旧数据往左移动，即可填充完
        else if (nNewBufferLen > nLeftCapacity)
        {
            // Trace("m_buffer.AutoRemove, nNewBufferLen:{},nLeftCapacity:{},nCurBufferLen:{},Capacity:{}",
            //       nNewBufferLen, nLeftCapacity, nCurBufferLen, nCapacity);
            m_buffer.AutoRemove();
            m_buffer.Append(buffer, len);
        }
        //剩余空间足够，新添加的数据直接追加到后面即可
        else // nNewBufferLen <=nLeftCapacity
        {
            m_buffer.Append(buffer, len);
        }

        return kNoError;
    }

    decoder_t::ErrorType decoder_t::Decode()
    {
        if (m_read_status == kReadHeader)
        {
            auto &packet = _GetPacket();
            packet.Clear();
            auto error = ParseHeader();
            if (error != kNoError)
            {
                return error;
            }
            m_read_status = kReadBody;
            m_howmuch = packet.m_uDataLength;
        }

        if (m_read_status == kReadBody)
        {
            auto error = ParseBody();
            if (error != kNoError)
            {
                return error;
            }
            m_read_status = kReadHeader;
            m_howmuch = sizeof(packet_t);
        }
        return kNoError;
    }

    decoder_t::ErrorType decoder_t::ParseHeader()
    {
        if (m_buffer.ReadableBytes() < m_howmuch)
        {
            return kNeedMoreData;
        }
        packet_t *packet = (packet_t *)(m_buffer.GetBuffer());
        if (packet->m_uHeadLength != sizeof(packet_t))
        {
            Error("ipc::decoder_t::ParseHeader, invalid uHeadLength,packet->m_uHeadLength={},fix size={}", packet->m_uHeadLength, sizeof(packet_t));
            return kInvalidHeader;
        }
        if (packet->m_uDataLength > m_packet_buffer.capacity()) //需要扩容来保存body数据
        {
            size_t uBufferCapacity = m_packet_buffer.capacity();
            size_t uNewBufferCapacity = std::min<size_t>(packet->m_uDataLength, uBufferCapacity * 2);
            Warn("ipc::decoder_t::ParseHeader, will reserve to store body data,body len={},capacity={},new capacity={}",
                 packet->m_uDataLength, uBufferCapacity, uNewBufferCapacity);
            m_packet_buffer.reserve(uNewBufferCapacity);
        }
        auto &my_packet = _GetPacket();
        memcpy(&my_packet, packet, sizeof(packet_t));
        m_buffer.Skip(m_howmuch);
        return kNoError;
    }
    decoder_t::ErrorType decoder_t::ParseBody()
    {
        if (m_buffer.ReadableBytes() < m_howmuch)
        {
            return kNeedMoreData;
        }
        auto &my_packet = _GetPacket();
        memcpy(my_packet.m_data, m_buffer.GetBuffer(), m_howmuch);
        m_buffer.Skip(m_howmuch);
        my_packet.m_data[m_howmuch] = '\0';
        return kNoError;
    }

} // namespace ipc