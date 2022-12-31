#include "WebServer.h"
#include "../../core/log/Log.hpp"
#include "../../core/ini_config.h"

WebServer::WebServer()
{
    m_rtmp_thread.Start("Rtmp_Thread");
}

bool WebServer::InitHttpHandlers()
{
    m_srs_on_session_handler = std::make_unique<HttpSrsOnSessionHandler>(this);
    m_web_on_play_handler = std::make_unique<HttpWebOnPlayHandler>(this);

    m_srs_on_session_handler->SetUri("/api/v1/sessions");
    m_web_on_play_handler->SetUri("/web_on_play");

    // 定时任务，当连接失败后，或者jt1078断开后，重连的时间间隔
    struct timeval connect_timer;
    connect_timer.tv_sec = 2;
    connect_timer.tv_usec = 0;
    m_connect_timer = std::make_unique<TimerEventWatcher>(GetEventLoop(), std::bind(&WebServer::OnConnectTimer, this), connect_timer);

    // 发送心跳包
    struct timeval heartbeat_timer;
    heartbeat_timer.tv_sec = 60;
    heartbeat_timer.tv_usec = 0;
    m_heartbeat_timer = std::make_unique<TimerEventWatcher>(GetEventLoop(), std::bind(&WebServer::OnHeartbeatTimer, this), heartbeat_timer);
    m_heartbeat_timer->StartTimer();

    return AsyncConnectServer();
}

// 这里的strDeviceId是标准的device_id格式，此函数调用之前会判断格式
// 因为在用户点击一个通道之后，就会触发一次。
// 所以存在触发多次，观看同一个通道的情况
void WebServer::NotifyStart(const std::string &strDeviceId)
{
    Info("WebServer::NotifyStart, strDeviceId:{}", strDeviceId);
    device_id_t device_id = m_device_id_mgr.GetDeviceIdFromStr(strDeviceId);

    if (m_device_id_mgr.Exists(device_id))
    {
        // 已经存在，则过滤掉
        return;
    }

    m_device_id_mgr.Insert(device_id);

    // 通知jt1078_server
    if (m_jt1078_client && m_jt1078_client->IsConnected())
    {
        m_jt1078_client->SendPacket(ipc::IPC_PKT_TYPE_SUBSCRIBE_DEVICE_ID | ipc::IPC_PKT_FROM_WEB_SERVER, strDeviceId.c_str(), strDeviceId.size());
    }

    // 通知子线程，创建对应的RTMP流
    ipc::packet_move_t move_packet;
    InitIpcMovePacket(move_packet, ipc::IPC_PKT_TYPE_SUBSCRIBE_DEVICE_ID | ipc::IPC_PKT_FROM_WEB_SERVER, strDeviceId.c_str(), strDeviceId.size());
    m_rtmp_thread.PostPacket(std::move(move_packet));
}

// 这里的strDeviceId是标准的device_id格式，此函数调用之前会判断格式
// 因为SRS回调都会带上client_id进行不同浏览器标识。HttpSrsOnSessionHandler处理了同一个通道不同client_id的情况
// 保证走到NotifyStop方法时，一个通道只会在需要关闭时触发一次。
// 所以可以直接调用Remove方法删除对应的device_id
void WebServer::NotifyStop(const std::string strDeviceId)
{
    Info("WebServer::NotifyStop, strDeviceId:{}", strDeviceId);
    device_id_t device_id = m_device_id_mgr.GetDeviceIdFromStr(strDeviceId);

    // 取消订阅
    m_device_id_mgr.Remove(device_id);

    // 通知jt1078_server
    if (m_jt1078_client && m_jt1078_client->IsConnected())
    {
        m_jt1078_client->SendPacket(ipc::IPC_PKT_TYPE_UNSUBSCRIBE_DEVICE_ID | ipc::IPC_PKT_FROM_WEB_SERVER, strDeviceId.c_str(), strDeviceId.size());
    }

    // 通知子线程，释放此通道的RTMP流
    ipc::packet_move_t move_packet;
    InitIpcMovePacket(move_packet, ipc::IPC_PKT_TYPE_UNSUBSCRIBE_DEVICE_ID | ipc::IPC_PKT_FROM_WEB_SERVER, strDeviceId.c_str(), strDeviceId.size());
    m_rtmp_thread.PostPacket(std::move(move_packet));
}

bool WebServer::AsyncConnectServer()
{
    m_jt1078_client.reset();

    const std::string forward_ip = g_ini->Get("jt1078", "forward_ip", "");
    const u_short forward_port = g_ini->GetInteger("jt1078", "forward_port", -1);
    int connect_timeout_second = g_ini->GetInteger("jt1078", "connect_timeout", 2);
    if (forward_ip.empty() || forward_port == -1)
    {
        Error("WebServer::AsyncConnectServer failed, forward_ip or forward_port invalid");
        return false;
    }

    m_jt1078_client = std::make_shared<Jt1078Client>();
    m_jt1078_client->SetHandlerOnConnect(std::bind(&WebServer::OnClientConnect, this, std::placeholders::_1, std::placeholders::_2));
    m_jt1078_client->SetHandlerOnMessage(std::bind(&WebServer::OnClientMessage, this, std::placeholders::_1, std::placeholders::_2));
    m_jt1078_client->SetHandlerOnError(std::bind(&WebServer::OnClientError, this, std::placeholders::_1, std::placeholders::_2));

    struct timeval connect_timeout;
    connect_timeout.tv_sec = connect_timeout_second;
    connect_timeout.tv_usec = 0;
    if (!m_jt1078_client->AsyncConnect(GetEventLoop(), forward_ip, forward_port, &connect_timeout))
    {
        Error("WebServer::AsyncConnectServer failed, client async_connect failed,ip:{},port:{}", forward_ip, forward_port);
        return false;
    }
    return true;
}

void WebServer::OnClientConnect(const Jt1078ClientPtr &client, bool bOk)
{
    Trace("WebServer::OnClientConnect, client session_id:{}, bOk:{}", client->GetSessionId(), bOk);
    if (!bOk)
    {
        m_jt1078_client.reset();
        m_connect_timer->StartTimer(); // 启动重连机制
        return;
    }

    // 重连成功，需要发送订阅请求，来获取数据
}

void WebServer::OnClientMessage(const Jt1078ClientPtr &client, const ipc::packet_t &packet)
{
    //  Trace("WebServer::OnClientMessage, sesion_id:{}, packet type:0x{:x},ipc_pkt.m_uHeadLength={},ipc_pkt.m_uPktSeqId={},ipc_pkt.m_uDataLength={}",
    //        client->GetSessionId(), packet.m_uPktType, packet.m_uHeadLength, packet.m_uPktSeqId, packet.m_uDataLength);

    // 接收到消息
    switch (packet.m_uPktType & ipc::IPC_PKT_TYPE_MASK)
    {
    case ipc::IPC_PKT_TYPE_JT1078_PACKET:
    {
        // 收到jt1078数据包后，添加到线程中
        m_rtmp_thread.PostPacket(packet);
        break;
    }
    default:
        break;
    }
}

void WebServer::OnClientError(const Jt1078ClientPtr &client, TcpErrorType error_type)
{
    Error("WebServer::OnClientError, client session_id:{},remote_ip:{},remote_port:{},fd:{},error_type:{},uLastReceiveIpcPktSeqId:{}",
          client->GetSessionId(), client->GetRemoteIp(), client->GetRemotePort(), client->GetSocketFd(), error_type, m_jt1078_client->GetLastReceiveIpcPktSeqId());
    m_jt1078_client.reset();
    m_connect_timer->StartTimer(); // 启动重连机制
}

void WebServer::OnConnectTimer()
{
    Trace("WebServer::OnConnectTimer");
    if (!AsyncConnectServer())
    {
        Error("WebServer::OnConnectTimer failed, AsyncConnectServer failed");
    }
}

void WebServer::OnHeartbeatTimer()
{
    // Trace("PcServer::OnHeartbeatTimer");
    if (m_jt1078_client && m_jt1078_client->IsConnected())
    {
        m_jt1078_client->SendPacket(ipc::IPC_PKT_TYPE_HEARTBEAT | ipc::IPC_PKT_FROM_WEB_SERVER, "", 0);
    }
    m_heartbeat_timer->StartTimer();
}

void WebServer::InitIpcMovePacket(ipc::packet_move_t &packet, uint32_t uPktType, const char *data, size_t len)
{
    packet.Clear();
    packet.m_uHeadLength = sizeof(ipc::packet_move_t);
    packet.m_uDataLength = len;
    packet.m_uPktSeqId = 0; // 后续完善递增
    packet.m_uPktType = uPktType;
    packet.m_data = new char[len + 1];
    memcpy(packet.m_data, data, len);
    packet.m_data[len] = '\0';
}
