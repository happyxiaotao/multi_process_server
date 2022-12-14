#ifndef FORWARD_SERVER_SUBSCRIBER_H
#define FORWARD_SERVER_SUBSCRIBER_H

#include <list>
#include <set>
#include "../../core/tcp/TcpSession.h"
#include "Message.h"
#include "../AV_Common_Define.h"
#include "../../core/ipc_packet/IpcPktDecoder.h"

namespace forward
{
    class Subscriber;
    typedef std::shared_ptr<Subscriber> SubscriberPtr;
    class Subscriber : public TcpSession
    {
        typedef std::function<void(const SubscriberPtr &subscriber, const ipc::packet_t &packet)> FunctorPacketComplete;
        typedef std::function<void(const SubscriberPtr &subscriber, TcpErrorType error)> FunctorOnError;

    public:
        Subscriber();
        virtual ~Subscriber() override;

    public:
        virtual bool Init(EventLoop *eventloop, int fd, const std::string &remote_ip, u_short remote_port) override;

        ssize_t SendMsg(const Message &message);
        inline void SetPacketComplete(const FunctorPacketComplete &functor) { m_functor_complete = functor; }
        inline void SetPacketError(const FunctorOnError &functor) { m_functor_error = functor; }

    private:
        virtual void OnMessage(const TcpSessionPtr &tcp, const Buffer &buffer) override;
        virtual void OnError(const TcpSessionPtr &tcp, TcpErrorType error_type) override;

    private:
        ssize_t SendMsg(const jt1078::packet_t &jt1078_pkt, device_id_t device_id);

        void InitIovecByPkt(struct iovec *iov, ipc::packet_t &ipc_pkt, const jt1078::packet_t &jt1078_pkt, const device_id_t &device_id);

        inline uint32_t GetIpcPktSeqId();

    private:
        FunctorPacketComplete m_functor_complete; // 一个完整包的处理函数
        FunctorOnError m_functor_error;

        ipc::decoder_t m_ipc_decoder;

        uint32_t m_uLastIpcPktSeqId;

        // std::set<device_id_t> m_setDeviceId; //订阅的device_id集合

        std::string m_send_buffer;
    };
} // namespace forward

#endif // FORWARD_SERVER_SUBSCRIBER_H