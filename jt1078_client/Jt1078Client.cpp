#include "Jt1078Client.h"
#include "../core/socket/Socket.h"
#include "../core/log/Log.hpp"

Jt1078Client::Jt1078Client() : m_uLastReceiveIpcPktSeqId(ipc::INVALID_PKT_SEQ_ID), m_uLastSendIpcPktSeqId(ipc::INVALID_PKT_SEQ_ID)
{
}
Jt1078Client::~Jt1078Client()
{
}

int Jt1078Client::SendPacket(ipc::IpcPktType type, const char *data, size_t len)
{
    m_ipc_packet.Clear();
    m_ipc_packet.m_uPktType = type;
    m_ipc_packet.m_uDataLength = len;
    m_ipc_packet.m_uPktSeqId = GetNewSendIpcPktSeqId();
    struct iovec iov[2];
    iov[0].iov_base = &m_ipc_packet;
    iov[0].iov_len = sizeof(ipc::packet_t);
    iov[1].iov_base = (void *)data;
    iov[1].iov_len = len;
    int nerrno = 0;
    ssize_t ret = SendBuffer(iov, sizeof(iov) / sizeof(iov[0]), nerrno);
    if (ret < 0)
    {
        Error("Jt1078Client::SendPacket failed, type:{},len:{},nerrno:{},error:{}", type, len, nerrno, strerror(nerrno));
    }
    return ret;
}

void Jt1078Client::OnConnected(evutil_socket_t fd, bool bOk, const char *err)
{
    Trace("Jt1078Client::OnConnected, fd:{},bOk:{},err:{}", fd, bOk, err);

    SetConnected(bOk);

    // 初始化EV_READ事件
    if (bOk)
    {
        InitReadEvent(fd);
    }

    if (m_functor_connect)
    {
        auto self = std::dynamic_pointer_cast<Jt1078Client>(shared_from_this());
        m_functor_connect(self, bOk);
    }
}
void Jt1078Client::OnMessage(const TcpSessionPtr &tcp, const Buffer &buffer)
{
    auto self = std::dynamic_pointer_cast<Jt1078Client>(tcp);
    m_ipc_decoder.PushBuffer(buffer);

    auto error = ipc::decoder_t::ErrorType::kNoError;
    while ((error = m_ipc_decoder.Decode()) == ipc::decoder_t::kNoError)
    {
        const auto &packet = m_ipc_decoder.GetPacket();
        m_uLastReceiveIpcPktSeqId = packet.m_uPktSeqId;
        if (m_functor_message)
        {
            m_functor_message(self, packet);
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
void Jt1078Client::OnError(const TcpSessionPtr &tcp, TcpErrorType error_type)
{
    SetConnected(false);

    if (m_functor_error)
    {
        auto self = std::dynamic_pointer_cast<Jt1078Client>(tcp);
        m_functor_error(self, error_type);
    }
}

uint32_t Jt1078Client::GetNewSendIpcPktSeqId()
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
