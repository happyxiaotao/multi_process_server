#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include "../../core/http_server/HttpServer.h"
#include "HttpSrsOnSessionHandler.h"
#include "HttpWebOnPlayHandler.h"
#include "../../jt1078_client/Jt1078Client.h"
#include "../../core/timer/TimerEventWatcher.h"
#include "DeviceIdMgr.h"
#include "../rtmp/RtmpThread.h"

class WebServer : public HttpServer
{
public:
    WebServer();
    virtual ~WebServer() {}

public:
    // HttpHandler进行初始化，设置相关回调
    virtual bool InitHttpHandlers() override;

public:
    // 外界调用，通知订阅此通道
    void NotifyStart(const std::string &strDeviceId);
    // 外界调用，通知停止此通道
    void NotifyStop(const std::string strDeviceId);

private:
    // 维护jt1078客户端的相关处理逻辑
    bool AsyncConnectServer();
    void OnClientConnect(const Jt1078ClientPtr &client, bool bOk);
    void OnClientMessage(const Jt1078ClientPtr &client, const ipc::packet_t &packet);
    void OnClientError(const Jt1078ClientPtr &client, TcpErrorType error_type);

    // 定时器回调函数
    void OnConnectTimer();
    void OnHeartbeatTimer();

    // ipc::packet_move_t的处理函数
    void InitIpcMovePacket(ipc::packet_move_t &packet, uint32_t uPktType, const char *data, size_t len);

private:
    // 两个Http处理函数
    std::unique_ptr<HttpSrsOnSessionHandler> m_srs_on_session_handler;
    std::unique_ptr<HttpWebOnPlayHandler> m_web_on_play_handler;

    // Jt1078客户端
    std::shared_ptr<Jt1078Client> m_jt1078_client;
    std::unique_ptr<TimerEventWatcher> m_connect_timer;   // 当连接失败时，定时重连的定时器
    std::unique_ptr<TimerEventWatcher> m_heartbeat_timer; // 心跳包定时任务

    DeviceIdMgr m_device_id_mgr;

    RtmpThread m_rtmp_thread; // RTMP流处理线程。主线程添加数据，子线程处理数据
};

#endif // WEB_SERVER_H