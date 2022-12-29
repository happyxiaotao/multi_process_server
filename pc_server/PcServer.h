#ifndef PC_SERVER_PC_SERVER_H
#define PC_SERVER_PC_SERVER_H

#include "../core/tcp/Listener.h"
#include "../core/eventloop/EventLoop.h"
#include "../jt1078_client/Jt1078Client.h"
#include "../core/timer/TimerEventWatcher.h"
#include "PcManager.h"
#include "PcPublisher.h"

class PcServer
{
public:
    PcServer();
    ~PcServer();

public:
    bool Init();
    void Start();

private:
    void OnNewConnection(evutil_socket_t socket, struct sockaddr *sa);
    void OnPacketCompleted(const PcSessionPtr &pc, const ipc::packet_t &packet);
    void OnPacketError(const PcSessionPtr &pc, TcpErrorType error_type);

    void OnClientConnect(const Jt1078ClientPtr &client, bool bOk);
    void OnClientMessage(const Jt1078ClientPtr &client, const ipc::packet_t &packet);
    void OnClientError(const Jt1078ClientPtr &client, TcpErrorType error_type);

    void OnConnectTimer();
    void OnHeartbeatTimer();

private:
    bool AsyncConnectServer();

private:
    // 下面是数据包的处理逻辑
    void ProcessPcPacket_Subscribe(const PcSessionPtr &pc, const ipc::packet_t &packet);
    void ProcessPcPacket_UnSubscribe(const PcSessionPtr &pc, const ipc::packet_t &packet);

private:
    EventLoop m_eventloop;
    std::unique_ptr<Listener> m_listener;
    std::string m_listen_ip;
    u_short m_listen_port;
    std::shared_ptr<Jt1078Client> m_client;
    std::unique_ptr<TimerEventWatcher> m_connect_timer;   // 当连接失败时，定时重连的定时器
    std::unique_ptr<TimerEventWatcher> m_heartbeat_timer; // 心跳包定时任务
    PcManager m_pc_manager;                               // 会话管理模块
    PcPublisher m_pc_publisher;
};
#endif // PC_SERVER_PC_SERVER_H