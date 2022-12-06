
#include <event2/event.h>
#include <unistd.h>
#include "RedisMqServer.h"
#include "RedisClient.h"
#include "../ini_config.h"
#include "../log/Log.hpp"
#include "../eventloop/EventLoop.h"

extern INIReader *g_ini;

CRedisMqServer::CRedisMqServer()
{
}
CRedisMqServer::~CRedisMqServer()
{
    Destory();
}
void CRedisMqServer::Destory()
{
    Trace("CRedisMqServer::Destory");
    m_ip.clear();
    m_port = 0;
    m_connect_timeout = 0;
    m_passport.clear();
    m_index = 0;
    m_redis_client.reset();
    m_eventloop = nullptr;
}
/*
void CRedisMqServer::SetPipeChannel(std::shared_ptr<PipeChannel> &&pipe_channel)
{
    m_pipe_channel = std::move(pipe_channel);
    m_pipe_channel->SetOnPipePktCompleted(std::bind(&CRedisMqServer::OnPipePktCompleted, this, std::placeholders::_1));
}
*/
bool CRedisMqServer::Init(EventLoop *eventloop)
{
    m_eventloop = eventloop;
    m_redis_client = std::make_unique<CRedisClient>();

    m_ip = std::move(g_ini->Get("redis", "ip", "127.0.0.1"));
    m_port = g_ini->GetInteger("redis", "port", 6379);
    m_connect_timeout = g_ini->GetInteger("redis", "connect_timeout", 2);
    m_passport = std::move(g_ini->Get("redis", "passport", ""));
    m_index = g_ini->GetInteger("redis", "index", 0);

    m_redis_client->Init(m_eventloop->GetEventBase(), this);
    if (!m_redis_client->AsyncConnect(m_ip, m_port, m_connect_timeout))
    {
        Destory();
        Error("CRedisMqServer::Init failed, RedisClient AsyncConnect failed!");
        return false;
    }

    struct timeval timer_timeout;
    timer_timeout.tv_sec = 1;
    timer_timeout.tv_usec = 0;
    m_timer = std::make_unique<TimerEventWatcher>(m_eventloop, std::bind(&CRedisMqServer::OnReconnectTimer, this), timer_timeout);
    if (!m_timer->Init())
    {
        Error("RedisMqServer Init failed, reconnect timer init failed");
        return false;
    }
    Trace("RedisMqServer Inited success. ip:{},port:{},index:{}", m_ip, m_port, m_index);

    return true;
}

void CRedisMqServer::Start()
{
    m_eventloop->Loop();
}

void CRedisMqServer::OnConnect(bool bOk)
{
    m_redis_client->SetConnected(bOk);
    if (bOk)
    {
        if (!m_passport.empty())
        {
            m_redis_client->Auth(m_passport);
        }
        else
        {
            Warn("Empty Redis Passport!");
            m_redis_client->Select(m_index);
        }
    }
    else
    {
        m_timer->StartTimer();
    }
}
// bOk=true:表示主动断开
// bOk=false：表示redis_server主动断开，需要重连
void CRedisMqServer::OnDisconnect(bool bOk)
{
    m_redis_client->SetConnected(false);
    if (bOk)
    {
        m_redis_client->SetConnected(false);
    }
    else
    {
        m_timer->StartTimer();
    }
}
void CRedisMqServer::OnAuth(redisReply *reply)
{
    if (reply->type == REDIS_REPLY_ERROR)
    {
        Error("redis auth failed!");
        m_redis_client->SetConnected(false);
        return;
    }
    Debug("redis auth ok!");
    m_redis_client->Select(m_index);
}
void CRedisMqServer::OnSelect(redisReply *reply)
{
    // 开始获取消息队列中数据
    if (reply->type == REDIS_REPLY_ERROR)
    {
        Error("redis select failed!");
        m_redis_client->SetConnected(false);
        return;
    }
    Debug("redis select ok!");
}

void CRedisMqServer::OnRpushList(SRedisFetchInfo *pInfo, redisReply *reply)
{
    if (reply->type == REDIS_REPLY_ERROR)
    {
        Error("CRedisMqServer::OnRpushList error, cmd:{}, error:{}", pInfo->m_lastcmd, reply->str);
        return;
    }
    Trace("CRedisMqServer::OnRpushList ok, cmd:{}!", pInfo->m_lastcmd);
}

void CRedisMqServer::RpushList(const std::string &strListName, const std::string &data)
{
    m_redis_client->RpushList(strListName, data);
}
void CRedisMqServer::OnReconnectTimer()
{
    if (m_redis_client->IsConnected())
    {
        return;
    }
    Error("redis client connect to redis failed, reconnecting..., ip:{},port:{},timeout:{}", m_ip, m_port, m_connect_timeout);
    m_redis_client->AsyncConnect(m_ip, m_port, m_connect_timeout);
}