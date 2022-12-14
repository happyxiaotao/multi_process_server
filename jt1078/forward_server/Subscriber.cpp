#include "Subscriber.h"
#include "../../core/log/Log.hpp"

namespace forward
{
    Subscriber::Subscriber() : m_bFdCanWrite(false), m_uLastIpcPktSeqId(ipc::INVALID_PKT_SEQ_ID)
    {
    }
    Subscriber::~Subscriber()
    {
    }
    bool Subscriber::Init(EventLoop *eventloop, int fd, const std::string &remote_ip, u_short remote_port)
    {
        if (TcpSession::Init(eventloop, fd, remote_ip, remote_port))
        {
            m_bFdCanWrite = true;
            return true;
        }
        return false;
    }
    void Subscriber::OnMessage(const TcpSessionPtr &tcp, const Buffer &buffer)
    {
        auto self = std::dynamic_pointer_cast<Subscriber>(tcp);
        auto error = m_ipc_decoder.PushBuffer(buffer);
        if (error == ipc::decoder_t::kBufferFull)
        {
            if (m_functor_error)
            {
                m_functor_error(self, TcpErrorType::TCP_ERROR_USER_BUFFER_FULL);
            }
            return;
        }

        while ((error = m_ipc_decoder.Decode()) == ipc::decoder_t::kNoError)
        {
            const auto &packet = m_ipc_decoder.GetPacket();
            if (m_functor_complete)
            {
                m_functor_complete(self, packet);
            }
        }
        if (error != ipc::decoder_t::kNeedMoreData)
        {
            if (m_functor_error)
            {
                m_functor_error(self, TcpErrorType::TCP_ERROR_INVALID_PACKET);
            }
        }
    }
    void Subscriber::OnError(const TcpSessionPtr &tcp, TcpErrorType error_type)
    {
        auto self = std::dynamic_pointer_cast<Subscriber>(tcp);
        if (m_functor_error)
        {
            m_functor_error(self, error_type);
        }
    }

    ssize_t Subscriber::SendMsg(const Message &message)
    {
        const jt1078::packet_t &jt1078_pkt = message.GetPkt();
        auto device_id = message.GetDeviceId();
        return SendMsg(jt1078_pkt, device_id);
    }

    ssize_t Subscriber::SendMsg(const jt1078::packet_t &jt1078_pkt, device_id_t device_id)
    {
        ipc::packet_t ipc_pkt;
        struct iovec iov[4];
        const int iovcnt = 4;
        InitIovecByPkt(iov, ipc_pkt, jt1078_pkt, device_id);
        return SendMsg(iov, iovcnt);
    }
    void Subscriber::AddPendingMsg(const Message &message)
    {
        const auto &jt1078_pkt = message.GetPkt();
        auto device_id = message.GetDeviceId();
        struct iovec iov[4];
        const int iovcnt = 4;
        ipc::packet_t ipc_pkt;
        InitIovecByPkt(iov, ipc_pkt, jt1078_pkt, device_id);
        AddPendingMsg(iov, iovcnt);
    }
    void Subscriber::AddPendingMsg(struct iovec *iov, int iovcnt)
    {
        std::string buffer;
        for (int i = 0; i < iovcnt; i++)
        {
            buffer.append((char *)iov[i].iov_base, iov[i].iov_len);
        }
        m_pending_buffer_list.emplace_back(buffer);
    }
    bool Subscriber::SendPendingMsg() // 发送未发送成功的数据包
    {
        while (!m_pending_buffer_list.empty())
        {
            const auto &buffer = m_pending_buffer_list.front();
            struct iovec iov[1];
            const size_t iovcnt = 1;
            iov[0].iov_base = (void *)buffer.c_str();
            iov[0].iov_len = buffer.size();
            const bool bInPendingList = true;
            if (SendMsg(iov, iovcnt, bInPendingList) < 0)
            {
                break;
            }
            m_pending_buffer_list.pop_front();
        }
        return !m_pending_buffer_list.empty();
    }

    ssize_t Subscriber::SendMsg(struct iovec *iov, int iovcnt, bool bInPendingList)
    {
        int nerrno = 0;
        int ret = SendBuffer(iov, iovcnt, nerrno);
        if (ret < 0)
        {
            Warn("Subscriber::SendMsg,SendBuffer failed, session_id:{}, errno:{},error:{}", GetSessionId(), nerrno, strerror(nerrno));
            // 未发送成功，将数据缓存起来
            if (nerrno == EAGAIN || nerrno == EWOULDBLOCK)
            {
                if (!bInPendingList) // 不在队列中，则追加
                {
                    AddPendingMsg(iov, iovcnt);
                }
            }
            else
            {
                m_bFdCanWrite = false;
            }
        }
        return ret;
    }
    // 注：device_id一定要传引用，不能传值，因为是在外界调用的iov进行发送，此时iov[1]的内容不确实啥。不是期望的效果。
    void Subscriber::InitIovecByPkt(struct iovec *iov, ipc::packet_t &ipc_pkt, const jt1078::packet_t &jt1078_pkt, const device_id_t &device_id)
    {
        ipc_pkt.m_uHeadLength = sizeof(ipc::packet_t);
        ipc_pkt.m_uDataLength = sizeof(device_id_t) + sizeof(jt1078::header_t) + jt1078_pkt.m_header->WdBodyLen;
        ipc_pkt.m_uPktSeqId = GetIpcPktSeqId();
        ipc_pkt.m_uPktType = ipc::IPC_PKT_JT1078_PACKET; // 注意中间插入了个device_id
        // 从1开始，0被ipc::packet_t占用了
        iov[0].iov_base = (void *)&ipc_pkt;
        iov[0].iov_len = sizeof(ipc::packet_t);
        iov[1].iov_base = (void *)&device_id;
        iov[1].iov_len = sizeof(device_id);
        iov[2].iov_base = jt1078_pkt.m_header;
        iov[2].iov_len = sizeof(jt1078::header_t);
        iov[3].iov_base = jt1078_pkt.m_body;
        iov[3].iov_len = jt1078_pkt.m_header->WdBodyLen;
        // Trace("session_id:{},device_id:{:14x},head len={},seq={},data len={}", GetSessionId(), device_id, ipc_pkt.m_uHeadLength, ipc_pkt.m_uPktSeqId, ipc_pkt.m_uDataLength);
    }

    uint32_t Subscriber::GetIpcPktSeqId()
    {
        if (m_uLastIpcPktSeqId == ipc::INVALID_PKT_SEQ_ID)
        {
            m_uLastIpcPktSeqId = 0;
        }
        else
        {
            ++m_uLastIpcPktSeqId;
        }
        return m_uLastIpcPktSeqId;
    }

} // namespace forward
