#include "ForwardServer.h"
#include "../../core/socket/Socket.h"
#include "../../core/log/Log.hpp"
#include "../jt1078_server/Jt1078Util.h"
#include "../Jt1078Service.h"
#include "../../3part/nonstd/string_view.hpp"

namespace forward
{
    ForwardServer::ForwardServer(EventLoop *eventloop, Jt1078Service *service) : m_eventloop(eventloop), m_service(service)
    {
        m_listener = std::make_shared<Listener>(m_eventloop);
        m_listener->SetConnectionCallBack(std::bind(&ForwardServer::OnNewConnection, this, std::placeholders::_1, std::placeholders::_2));
    }
    ForwardServer::~ForwardServer()
    {
    }
    void ForwardServer::OnNewConnection(evutil_socket_t socket, struct sockaddr *sa)
    {
        std::string ip;
        u_short port;
        sock::GetIpPortFromSockaddr(*(struct sockaddr_in *)(sa), ip, port);
        Info("ForwardServer::OnNewConnection, socket={},ip={},port={}", socket, ip, port);

        auto subscriber = std::make_shared<Subscriber>();
        if (!subscriber->Init(m_eventloop, socket, ip, port))
        {
            Error("ForwardServer::OnNewConnection failed, subscriber init failed, remote_ip:{},remote_port:{},fd:{}", ip, port, socket);
            return;
        }
        subscriber->SetPacketComplete(std::bind(&ForwardServer::OnPacketCompleted, this, std::placeholders::_1, std::placeholders::_2));
        subscriber->SetPacketError(std::bind(&ForwardServer::OnPacketError, this, std::placeholders::_1, std::placeholders::_2));
        m_mapSubscriber[subscriber->GetSessionId()] = subscriber; // 当发送订阅请求时，再放入对应的订阅列表
        Info("ForwardServer::OnNewConnection,m_mapSubscriber.size()={}", m_mapSubscriber.size());
    }

    bool ForwardServer::Listen(const std::string &ip, u_short port)
    {
        Info("ForwardServer::Listen,ip={},port={}", ip, port);
        return m_listener->Listen(ip, port);
    }
    void ForwardServer::Publish(device_id_t device_id, const jt1078::packet_t &pkt)
    {
        // auto seq = pkt.m_header->WdPackageSequence;
        // Trace("ForwardServer::Publish, pkt->seq:{}", seq);

        // 找到对应的通道进行数据推送
        auto channel_id = device_id;
        const Message message(pkt, device_id);
        Publisher::Publish(channel_id, message);
    }

    // 当前的发布订阅模式：订阅者发送订阅某个通道（即device_id）的请求。然后通道channel保存此会话，在publisher发布时发送数据
    // 一个订阅者，可以订阅多个通道
    void ForwardServer::OnPacketError(const SubscriberPtr &subscriber, TcpErrorType error_type)
    {
        Error("ForwardServer::OnPacketError, subscriber session_id:{},error_type:{}", subscriber->GetSessionId(), error_type);
        m_mapSubscriber.erase(subscriber->GetSessionId());
        Info("ForwardServer::OnPacketError,m_mapSubscriber.size()={}", m_mapSubscriber.size());
        // 从所有的通道中删除此订阅者
        DelSubscriber(subscriber);

        // 当某个通道订阅列表为空时，发送断开连接，并释放此通道
        ReleaseChannelIfEmptySubscriber();
    }
    void ForwardServer::OnPacketCompleted(const SubscriberPtr &subscriber, const ipc::packet_t &packet)
    {
        switch (packet.m_uPktType)
        {
        case ipc::IPC_PKT_HEARTBEAT:
            break;
        case ipc::IPC_PKT_SUBSCRIBE_DEVICD_ID:
            ProcessPacket_Subscriber(subscriber, packet);
            break;
        case ipc::IPC_PKT_UNSUBSCRIBE_DEVICD_ID:
            ProcessPacket_UnSubscriber(subscriber, packet);
            break;
        default:
            break;
        }
    }
    void ForwardServer::ProcessPacket_Subscriber(const SubscriberPtr &subscriber, const ipc::packet_t &packet)
    {
        device_id_t device_id = GenerateDeviceIdByBuffer(packet.m_data);
        Info("ForwardServer::OnPacketCompleted,subscriber session_id:{},device_id:{:014x}", subscriber->GetSessionId(), device_id);
        channel_id_t channel_id = device_id; // device_id和channel_id一样。即每个终端都是一个通道
        // 通道之前不存在
        if (!Exists(channel_id))
        {
            Info("ForwardServer::ProcessRequest_Subscriber,Create Channel,channel_id:{:x}", channel_id);
            CreateChannel(channel_id);
            NotifyOpenCarTerminal(device_id);
        }
        AddSubscriber(channel_id, subscriber);
    }

    void ForwardServer::ProcessPacket_UnSubscriber(const SubscriberPtr &subscriber, const ipc::packet_t &packet)
    {
        device_id_t device_id = GenerateDeviceIdByBuffer(packet.m_data);
        Info("ForwardServer::ProcessPacket_UnSubscriber,subscriber session_id:{},device_id:{:014x}", subscriber->GetSessionId(), device_id);
        channel_id_t channel_id = device_id;
        DelSubscriber(channel_id, subscriber);
        ReleaseChannelIfEmptySubscriber();
    }

    void ForwardServer::NotifyOpenCarTerminal(device_id_t device_id)
    {
        NotifyOpenCarTerminal(GenerateDeviceIdStrByDeviceId(device_id));
    }
    void ForwardServer::NotifyOpenCarTerminal(const std::string &strDeviceId)
    {
        constexpr bool bConnect = true;
        m_service->SendCommandTo808(strDeviceId, bConnect);
    }

    void ForwardServer::NotifyCloseCarTerminal(device_id_t device_id)
    {
        NotifyCloseCarTerminal(GenerateDeviceIdStrByDeviceId(device_id));
    }
    void ForwardServer::NotifyCloseCarTerminal(const std::string &strDeviceId)
    {
        constexpr bool bConnect = false;
        m_service->SendCommandTo808(strDeviceId, bConnect);
        m_service->NotifyNoSubscriber(strDeviceId);
    }

    // 如果订阅列表为空，则向redis发送断开请求
    void ForwardServer::ReleaseChannelIfEmptySubscriber()
    {
        for (auto iter = m_mapChannel.begin(); iter != m_mapChannel.end();)
        {
            auto &channel = iter->second;
            if (channel->Empty())
            {
                device_id_t device_id = channel->GetChannelId(); // device_id和channel_id相同
                Trace("ForwardServer::ReleaseChannelIfEmptySubscriber, device_id={:014x}", device_id);
                NotifyCloseCarTerminal(device_id);
                iter = m_mapChannel.erase(iter);
            }
            else
            {
                ++iter;
            }
        }
    }

} // namespace forward
