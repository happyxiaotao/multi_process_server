#ifndef _REDIS_MQ_SERVER_H
#define _REDIS_MQ_SERVER_H
#include <string>
#include <memory>

#include "../timer/TimerEventWatcher.h"

// 专门处理redis队列的服务。
// 队列中既有
// redis服务和主进程放在同一个地方
// 当有需要时，再创建多线程模型

struct event_base;
struct event;
class CRedisClient;
struct redisReply;
struct SRedisFetchInfo;
class EventLoop;

class CRedisMqServer
{
public:
    CRedisMqServer();
    ~CRedisMqServer();

public:
    bool Init(EventLoop *eventloop);
    void Start();

public:
    void OnConnect(bool bOk);
    void OnDisconnect(bool bOk);
    void OnAuth(redisReply *reply);
    void OnSelect(redisReply *reply);

    void OnRpushList(SRedisFetchInfo *pInfo, redisReply *reply);
    // void OnLpopList(SRedisFetchInfo *pInfo, redisReply *reply);

    void RpushList(const std::string &strListName, const std::string &data);

private:
    void Destory();

    void OnReconnectTimer();

private:
    std::string m_ip;
    u_short m_port;
    size_t m_connect_timeout;
    std::string m_passport;
    size_t m_index;

    std::unique_ptr<CRedisClient> m_redis_client;
    EventLoop *m_eventloop;

    // 定时重连逻辑
    std::unique_ptr<TimerEventWatcher> m_timer;
};

#endif //_REDIS_MQ_SERVER_H