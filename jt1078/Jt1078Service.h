#ifndef JT1078_SERVICE_H
#define JT1078_SERVICE_H

#include "./jt1078_server/Jt1078Server.h"
#include "./forward_server/ForwardServer.h"
#include "../core/eventloop/EventLoop.h"
#include "../core/database/RedisMqServer.h"

class Jt1078Service
{
public:
    Jt1078Service();
    ~Jt1078Service();

public:
    bool Init();
    bool Start();

public:
    void Notify1078Packet(device_id_t device_id, const jt1078::packet_t &pkt, bool bRealtime);

    // bConnect=true 向808服务器发送消息，通知汽车连接本服务器
    // bConnect=false 向808服务器发送消息，通知汽车断开本服务器连接
    void SendCommandTo808(const std::string &strDeviceId, bool bConnect, bool bRealtime);

    // 提示没有订阅者了
    void NotifyNoSubscriber(const std::string &strDeviceId, bool bRealtime);

private:
    std::unique_ptr<Jt1078Server> m_jt1078_realtime_server;            // 实时视频服务器
    std::unique_ptr<Jt1078Server> m_jt1078_history_server;             // 历史视频服务器
    std::unique_ptr<forward::ForwardServer> m_forward_realtime_server; // 实时数据转发服务
    std::unique_ptr<forward::ForwardServer> m_forward_history_server;  // 历史数据转发服务

    std::unique_ptr<CRedisMqServer> m_redis_server;

    std::string m_redis_808_list;
    EventLoop m_eventloop;
};

#endif // JT1078_SERVICE_H