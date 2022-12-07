#ifndef JT1078_FORWARD_SERVER_H
#define JT1078_FORWARD_SERVER_H

#include "../../core/tcp/Listener.h"
#include "../jt1078_server/Jt1078Packet.h"
#include "../AV_Common_Define.h"
#include "Publisher.h"

class Jt1078Service;
namespace forward
{
    class ForwardServer : public Publisher
    {
    public:
        ForwardServer(EventLoop *eventloop, Jt1078Service *service);
        virtual ~ForwardServer() override;

    public:
        void OnNewConnection(evutil_socket_t socket, struct sockaddr *sa);

        bool Listen(const std::string &ip, u_short port);

        void OnPacketError(const SubscriberPtr &subscriber, TcpErrorType error_type);
        void OnPacketCompleted(const SubscriberPtr &subscriber, const ipc::packet_t &packet);

        void Publish(device_id_t device_id, const jt1078::packet_t &pkt);

    private:
        // 处理订阅请求
        void ProcessPacket_Subscriber(const SubscriberPtr &subscriber, const ipc::packet_t &packet);
        // 处理取消订阅
        void ProcessPacket_UnSubscriber(const SubscriberPtr &subscriber, const ipc::packet_t &packet);

    private:
        // 通知主服务，去向808服务器发送消息，让汽车连过来或断开连接
        void NotifyOpenCarTerminal(device_id_t device_id);
        void NotifyOpenCarTerminal(const std::string &strDeviceId);
        void NotifyCloseCarTerminal(device_id_t device_id);
        void NotifyCloseCarTerminal(const std::string &strDeviceId);

        // 如果订阅列表为空，则向redis发送断开请求
        void ReleaseChannelIfEmptySubscriber();

    private:
        EventLoop *m_eventloop;
        ListenerPtr m_listener;

        std::map<session_id_t, SubscriberPtr> m_mapSubscriber; //保存所有订阅者

        Jt1078Service *m_service;
    };
} // namespace forward

#endif // JT1078_FORWARD_SERVER_H