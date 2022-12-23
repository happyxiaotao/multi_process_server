#include "PcSession.h"
#include "../core/log/Log.hpp"

PcSession::PcSession() : m_uLastReceiveIpcPktSeqId(ipc::INVALID_PKT_SEQ_ID),
                         m_uLastSendIpcPktSeqId(ipc::INVALID_PKT_SEQ_ID)
{
}

PcSession::~PcSession()
{
}

void PcSession::OnMessage(const TcpSessionPtr &tcp, const Buffer &buffer)
{
    auto self = std::dynamic_pointer_cast<PcSession>(tcp);
    m_ipc_decoder.PushBuffer(buffer);
    auto error = ipc::decoder_t::ErrorType::kNoError;
    while ((error = m_ipc_decoder.Decode()) == ipc::decoder_t::kNoError)
    {
        const auto &packet = m_ipc_decoder.GetPacket();
        m_uLastReceiveIpcPktSeqId = packet.m_uPktSeqId;
        if (m_functor_complete)
        {
            m_functor_complete(self, packet);
        }
    }
    if (error != ipc::decoder_t::ErrorType::kNeedMoreData)
    {
        if (m_functor_error)
        {
            m_functor_error(self, TcpErrorType::TCP_ERROR_INVALID_PACKET);
        }
    }
}

void PcSession::OnError(const TcpSessionPtr &tcp, TcpErrorType error_type)
{
    if (m_functor_error)
    {
        auto self = std::dynamic_pointer_cast<PcSession>(tcp);
        m_functor_error(self, error_type);
    }
}

uint32_t PcSession::GetNewSendIpcPktSeqId()
{
    if (m_uLastSendIpcPktSeqId == ipc::INVALID_PKT_SEQ_ID)
    {
        m_uLastSendIpcPktSeqId = 0;
    }
    else
    {
        ++m_uLastSendIpcPktSeqId;
    }
    return m_uLastSendIpcPktSeqId;
}
bool PcSession::SendPacket(const ipc::packet_t &packet)
{
    // struct iovec iov[2];
    // iov[0].iov_base = (char *)&packet;
    // iov[0].iov_len = sizeof(packet);
    // iov[1].iov_base = (void *)packet.m_data;
    // iov[1].iov_len = packet.m_uDataLength;
    // int nerrno = 0;
    // auto ret = SendBuffer(iov, 2, nerrno);
    //  使用writev函数存在，ret<0或没有写完整的额情况，在Qt客户端多路视频播放时，容易出现。导致Qt客户端解析数据包异常，断开连接，无法继续播放视频
    //  如果对writev进行返回值判断， 然后添加到缓存中的话，比较麻烦。所以使用write函数，进行代替。当出现数据发送失败时，直接拷贝到buffer中。下次继续发送

    // 优先发送缓存的Buffer
    if (!EmptyPendingBuffer())
    {
        bool bOk = SendPendingBuffer();
        if (!bOk) // 没有发送完
        {
            if (!IsCanWrite()) // 对端socket无法继续写入（非EAGAIN和EWOULDBLOCK错误）
            {
                return false; // 对端可能close了，无法继续写入。
            }
            else
            {
                // 将准备写入的数据，拷贝到缓存中
                PushBackPendingBuffer((char *)&packet, sizeof(packet) + packet.m_uDataLength);
                return true;
            }
        }
    }

    size_t expect_write_len = sizeof(ipc::packet_t) + packet.m_uDataLength;
    int nerrno = 0;
    auto ret = SendBuffer((char *)&packet, expect_write_len, nerrno);
    if (ret < 0)
    {
        Trace("session_id:{},device_id:{},head len={},seq={},data len={},ret={},nerrno={}",
              GetSessionId(), GetDeviceId(), packet.m_uHeadLength, packet.m_uPktSeqId, packet.m_uDataLength, ret, nerrno);
        if (IsCanWrite())
        {
            // 新数据添加到列表后面
            PushBackPendingBuffer((char *)&packet, sizeof(ipc::packet_t) + packet.m_uDataLength);
        }
        else // 对端socket已经关闭，此时还未回调on_read，进行资源释放
        {
            //     Trace("session_id:{},device_id:{},head len={},seq={},data len={},ret={},nerrno={}",
            //           GetSessionId(), GetDeviceId(), packet.m_uHeadLength, packet.m_uPktSeqId, packet.m_uDataLength, ret, nerrno);
            return false;
        }
    }
    else if (ret < (ssize_t)expect_write_len) // 没有发送完
    {
        size_t left_size = expect_write_len - ret;
        Trace("PcSession::SendPacket, SendBuffer exists left,expect_write_len:{},ret:{},left size:{},session_id:{},device_id:{}", expect_write_len, ret, left_size, GetSessionId(), GetDeviceId());
        // 将剩余的填充
        const char *p = (char *)(&packet) + ret;
        PushBackPendingBuffer(p, left_size);
    }
    else
    {
        //  Trace("session_id:{},device_id:{},head len={},seq={},data len={},ret={},nerrno={}",
        //        GetSessionId(), GetDeviceId(), packet.m_uHeadLength, packet.m_uPktSeqId, packet.m_uDataLength, ret, nerrno);
    }

    return true;
}
