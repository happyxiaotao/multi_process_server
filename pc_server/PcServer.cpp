#include "PcServer.h"
#include "../core/socket/Socket.h"
#include "../core/log/Log.hpp"
#include "../core/ini_config.h"
#include "../jt1078/AV_Common_Define.h"
#include "../jt1078/jt1078_server/Jt1078Util.h"

PcServer::PcServer() : m_listen_port(INVALID_PORT)
{
}

PcServer::~PcServer()
{
    m_listener.reset();
    m_client.reset();
    // 需要在在m_eventloop析构之前，将相关依赖项资源先释放
}

bool PcServer::Init()
{
    if (!m_eventloop.Init())
    {
        Error("PcServer::Init failed, eventloop init failed");
        return false;
    }

    m_listener = std::make_unique<Listener>(&m_eventloop);
    m_listener->SetConnectionCallBack(std::bind(&PcServer::OnNewConnection, this, std::placeholders::_1, std::placeholders::_2));

    const std::string ip = g_ini->Get("pc", "ip", "");
    const u_short port = g_ini->GetInteger("pc", "port", INVALID_PORT);
    if (ip.empty() || port == INVALID_PORT)
    {
        Error("PcServer::Init failed, ip or port invalid");
        return false;
    }
    if (!m_listener->Listen(ip, port))
    {
        Error("PcServer::Init failed, listen failed,ip:{},port:{}", ip, port);
        return false;
    }
    m_listen_ip = ip;
    m_listen_port = port;

    // 定时任务，当连接失败后，或者jt1078断开后，重连的时间间隔
    struct timeval connect_timer;
    connect_timer.tv_sec = 2;
    connect_timer.tv_usec = 0;
    m_connect_timer = std::make_unique<TimerEventWatcher>(&m_eventloop, std::bind(&PcServer::OnConnectTimer, this), connect_timer);

    struct timeval heartbeat_timer;
    heartbeat_timer.tv_sec = 60;
    heartbeat_timer.tv_usec = 0;
    m_heartbeat_timer = std::make_unique<TimerEventWatcher>(&m_eventloop, std::bind(&PcServer::OnHeartbeatTimer, this), heartbeat_timer);
    m_heartbeat_timer->StartTimer();

    return AsyncConnectServer();
}
void PcServer::Start()
{
    m_eventloop.Loop();
}

void PcServer::OnNewConnection(evutil_socket_t socket, struct sockaddr *sa)
{
    std::string ip;
    u_short port;
    sock::GetIpPortFromSockaddr(*(struct sockaddr_in *)(sa), ip, port);
    Info("PcServer::OnNewConnection, remote_ip={},remote_port={},fd={}", ip, port, socket);

    auto pc = std::make_shared<PcSession>();
    if (!pc->Init(&m_eventloop, socket, ip, port))
    {
        Error("PcServer::OnNewConnection failed, pc init failed, remote_ip:{},remote_port:{},fd:{}", ip, port, socket);
        return;
    }
    pc->SetPacketComplete(std::bind(&PcServer::OnPacketCompleted, this, std::placeholders::_1, std::placeholders::_2));
    pc->SetPacketError(std::bind(&PcServer::OnPacketError, this, std::placeholders::_1, std::placeholders::_2));
    m_pc_manager.AddPc(pc);
}

void PcServer::OnPacketCompleted(const PcSessionPtr &pc, const ipc::packet_t &packet)
{
    Trace("PcServer::OnPacketCompleted, pc session_id:{}, packet_type:{}", pc->GetSessionId(), packet.m_uPktType);
    if (!m_client->IsConnected())
    {
        Error("PcServer::OnPacketCompleted,can not handler pc msg, jt1078 server is not connected,session_id:{},packet_type:{}",
              pc->GetSessionId(), packet.m_uPktType);
        return;
    }

    switch (packet.m_uPktType)
    {
    case ipc::IPC_PKT_SUBSCRIBE_ICCID:
        ProcessPcPacket_Subscribe(pc, packet);
        break;
    case ipc::IPC_PKT_UNSUBSCRIBE_ICCID:
        ProcessPcPacket_UnSubscribe(pc, packet);
        break;
    default:
        break;
    }
}
void PcServer::OnPacketError(const PcSessionPtr &pc, TcpErrorType error_type)
{
    Trace("PcServer::OnPacketError, pc session_id:{}, error_type:{}", pc->GetSessionId(), error_type);
    m_pc_publisher.DelSubscriber(pc);
    m_pc_manager.DelPc(pc);

    // 目前限制，一个PcSession只能有订阅一个iccid
    const auto &strIccid = pc->GetIccid();
    if (m_client->IsConnected())
    {
        m_client->SendPacket(ipc::IPC_PKT_UNSUBSCRIBE_ICCID, strIccid.c_str(), strIccid.size());
    }
}

void PcServer::OnClientConnect(const Jt1078ClientPtr &client, bool bOk)
{
    Trace("PcServer::OnClientConnect, client session_id:{}, bOk:{}", client->GetSessionId(), bOk);
    if (!bOk)
    {
        m_connect_timer->StartTimer(); //启动重连机制
        return;
    }
}

void PcServer::OnClientMessage(const Jt1078ClientPtr &client, const ipc::packet_t &packet)
{
    //   Trace("PcServer::OnClientMessage, sesion_id:{}, packet type:{},ipc_pkt.m_uHeadLength={},ipc_pkt.m_uPktSeqId={},ipc_pkt.m_uDataLength={}",
    //         client->GetSessionId(), packet.m_uPktType, packet.m_uHeadLength, packet.m_uPktSeqId, packet.m_uDataLength);

    if (packet.m_uPktType == ipc::IPC_PKT_JT1078_PACKET)
    {
        iccid_t iccid;
        memcpy(&iccid, packet.m_data, sizeof(iccid));
        m_pc_publisher.Publish(iccid, packet);
    }
}
void PcServer::OnClientError(const Jt1078ClientPtr &client, TcpErrorType error_type)
{
    Error("PcServer::OnClientError, client session_id:{},remote_ip:{},remote_port:{},fd:{},error_type:{},uLastReceiveIpcPktSeqId:{}",
          client->GetSessionId(), client->GetRemoteIp(), client->GetRemotePort(), client->GetSocketFd(), error_type, m_client->GetLastReceiveIpcPktSeqId());
    m_client.reset();
    m_connect_timer->StartTimer(); //启动重连机制
}
void PcServer::OnConnectTimer()
{
    Trace("PcServer::OnConnectTimer");
    if (!AsyncConnectServer())
    {
        Error("PcServer::OnConnectTimer failed, AsyncConnectServer failed");
    }
}
void PcServer::OnHeartbeatTimer()
{
    Trace("PcServer::OnHeartbeatTimer");
    if (m_client->IsConnected())
    {
        m_client->SendPacket(ipc::IPC_PKT_HEARTBEAT, "", 0);
    }
    m_heartbeat_timer->StartTimer();
}
bool PcServer::AsyncConnectServer()
{
    if (m_client)
    {
        m_client.reset();
    }

    const std::string forward_ip = g_ini->Get("jt1078", "forward_ip", "");
    const u_short forward_port = g_ini->GetInteger("jt1078", "forward_port", INVALID_PORT);
    int connect_timeout_second = g_ini->GetInteger("jt1078", "connect_timeout", 2);
    if (forward_ip.empty() || forward_port == INVALID_PORT)
    {
        Error("PcServer::AsyncConnectServer failed, forward_ip or forward_port invalid");
        return false;
    }

    m_client = std::make_shared<Jt1078Client>();
    m_client->SetHandlerOnConnect(std::bind(&PcServer::OnClientConnect, this, std::placeholders::_1, std::placeholders::_2));
    m_client->SetHandlerOnMessage(std::bind(&PcServer::OnClientMessage, this, std::placeholders::_1, std::placeholders::_2));
    m_client->SetHandlerOnError(std::bind(&PcServer::OnClientError, this, std::placeholders::_1, std::placeholders::_2));

    struct timeval connect_timeout;
    connect_timeout.tv_sec = connect_timeout_second;
    connect_timeout.tv_usec = 0;
    if (!m_client->AsyncConnect(&m_eventloop, forward_ip, forward_port, &connect_timeout))
    {
        Error("PcServer::AsyncConnectServer failed, client async_connect failed,ip:{},port:{}", forward_ip, forward_port);
        return false;
    }
    return true;
}

void PcServer::ProcessPcPacket_Subscribe(const PcSessionPtr &pc, const ipc::packet_t &packet)
{
    std::string strIccid(packet.m_data, packet.m_uDataLength);
    Trace("pc subscribe iccid:{}, session_id:{}", strIccid, pc->GetSessionId());

    // 如果存在旧的iccid，则需要通知取消订阅旧的数据包
    const std::string &strOldIccid = pc->GetIccid();
    if (!strOldIccid.empty() && strOldIccid != strIccid)
    {
        m_client->SendPacket(ipc::IPC_PKT_UNSUBSCRIBE_ICCID, strOldIccid.c_str(), strOldIccid.size());
    }
    pc->SetIccid(strIccid);
    if (!strIccid.empty())
    {
        iccid_t iccid = GenerateIccid(strIccid);
        m_pc_publisher.AddSubscriber(iccid, pc);
        m_client->SendPacket(ipc::IPC_PKT_SUBSCRIBE_ICCID, strIccid.c_str(), strIccid.size());
    }
}
void PcServer::ProcessPcPacket_UnSubscribe(const PcSessionPtr &pc, const ipc::packet_t &packet)
{
    const auto &strIccid = pc->GetIccid();
    Trace("pc unsubscribe iccid:{}, session_id:{}", strIccid, pc->GetSessionId());
    if (!strIccid.empty())
    {
        iccid_t iccid = GenerateIccid(strIccid);
        m_pc_publisher.DelSubscriber(iccid, pc);
        m_client->SendPacket(ipc::IPC_PKT_UNSUBSCRIBE_ICCID, strIccid.c_str(), strIccid.size());
    }
}