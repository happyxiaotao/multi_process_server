#include "PcSession.h"

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
ssize_t PcSession::SendPacket(const ipc::packet_t &packet)
{
    struct iovec iov[2];
    iov[0].iov_base = (char *)&packet;
    iov[0].iov_len = sizeof(packet);
    iov[1].iov_base = (void *)packet.m_data;
    iov[1].iov_len = packet.m_uDataLength;
    int nerrno = 0;
    return SendBuffer(iov, 2, nerrno);
}
