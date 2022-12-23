#include "Subscriber.h"
#include "../../core/log/Log.hpp"

namespace forward
{
    Subscriber::Subscriber() : m_uLastIpcPktSeqId(ipc::INVALID_PKT_SEQ_ID)
    {
    }
    Subscriber::~Subscriber()
    {
    }
    bool Subscriber::Init(EventLoop *eventloop, int fd, const std::string &remote_ip, u_short remote_port)
    {
        if (TcpSession::Init(eventloop, fd, remote_ip, remote_port))
        {
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
        m_send_buffer.clear();

        ipc::packet_t ipc_pkt;
        struct iovec iov[4];
        InitIovecByPkt(iov, ipc_pkt, jt1078_pkt, device_id);
        for (int i = 0; i < 4; i++)
        {
            m_send_buffer.append((char *)iov[i].iov_base, iov[i].iov_len);
        }

        if (!EmptyPendingBuffer())
        {
            Warn("Subscriber::SendMsg, Handler Pending Msg, pending size:{}", SizePendingBuffer());
            bool bOk = SendPendingBuffer(); // 发送缓存的数据
            if (!bOk)                       // 没有发送完
            {
                if (IsCanWrite()) // socket可写，则将本次将要写入的数据，拷贝到缓存区中
                {
                    PushBackPendingBuffer(std::move(std::string(m_send_buffer))); // 注意 std::move
                    return m_send_buffer.size();
                }
                else // socket不可写
                {
                    return -1;
                }
            }
        }
        // 走到这里，说明是EmptyPendingBuffer
        int nerrno = 0;
        int ret = SendBuffer(m_send_buffer.c_str(), m_send_buffer.size(), nerrno);
        if (ret < 0)
        {
            if (nerrno == EAGAIN || nerrno == EWOULDBLOCK)
            {
                Warn("Subscriber::SendMsg, SendBuffer return -1,errno=EAGAIN,push pendingbuffrer,session_id:{},device_id:{}", GetSessionId(), device_id);
                PushBackPendingBuffer(std::move(std::string(m_send_buffer)));
            }
            else // 对端不可用
            {
                Warn("Subscriber::SendMsg, SendBuffer return -1,errno={},error:{},return false,session_id:{},device_id:{}", nerrno, strerror(nerrno), GetSessionId(), device_id);
                return -1;
            }
        }
        else if (ret < (ssize_t)m_send_buffer.size())
        {
            Warn("Subscriber::SendMsg, SendBuffer return {},expect value:{},session_id:{},device_id:{}", ret, m_send_buffer.size(), strerror(nerrno), GetSessionId(), device_id);
            PushBackPendingBuffer(m_send_buffer.c_str() + ret, m_send_buffer.size() - ret);
            return ret;
        }
        else // ret == nCurNeedWriteSize
        {
            return m_send_buffer.size();
        }
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
