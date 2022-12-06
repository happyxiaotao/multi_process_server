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
    void Notify1078Packet(iccid_t iccid, const jt1078::packet_t &pkt);

public:
    // bConnect=true 向808服务器发送消息，通知汽车连接本服务器
    // bConnect=false 向808服务器发送消息，通知汽车断开本服务器连接
    void SendCommandTo808(const std::string &strIccid, bool bConnect);

private:
    std::unique_ptr<Jt1078Server> m_jt1078_server;
    std::unique_ptr<forward::ForwardServer> m_forward_server;
    std::unique_ptr<CRedisMqServer> m_redis_server;

    bool m_is_history_server; //目前暂时通过此变量来设置是历史还是实时视频。后续考虑历史和实时合并起来。
    std::string m_redis_808_list;
    EventLoop m_eventloop;
};

#endif // JT1078_SERVICE_H