
#include "RedisClient.h"
#include "RedisMqServer.h"
#include "../log/Log.hpp"

static void RedisConnectCallback(const redisAsyncContext *c, int status)
{
    if (status != REDIS_OK)
    {
        // 连接失败，后续考虑重连的情况
        Error("RedisConnectCallback connect to redis failed, status={},error:{}", status, c->errstr);
    }
    else
    {
        Info("RedisConnectCallback connect to redis success!");
    }
    CRedisMqServer *pServer = (CRedisMqServer *)c->data;
    pServer->OnConnect(status == REDIS_OK);
}
static void RedisDisconnectCallback(const redisAsyncContext *c, int status)
{
    if (status != REDIS_OK) // 说明对端连接有问题主动断开连接。这种清下，需要考虑redis重连的情况
    {
        Error("RedisDisconnectCallback disconnect to redis, status={},error:{}", status, c->errstr);
    }
    else
    {
        Trace("RedisDisconnectCallback disconnect to redis success!");
    }
    CRedisMqServer *pServer = (CRedisMqServer *)c->data;
    pServer->OnDisconnect(status == REDIS_OK);
}

void RedisCommandCallback(redisAsyncContext *c, void *r, void *privdata)
{
    SRedisFetchInfo *pInfo = (SRedisFetchInfo *)privdata;
    redisReply *reply = (redisReply *)r;

    if (reply == nullptr)
    {
        Error("RedisCommandCallback pInfo->type:{}, cmd:{}, reply=nullptr", pInfo->m_type, pInfo->m_lastcmd);
        return;
    }
    else
    {
        //    Trace("RedisCommandCallback, pInfo->type:{}, cmd:{}, reply->type:{}", pInfo->m_type, pInfo->m_lastcmd, reply->type);
    }

    if (reply->type == REDIS_REPLY_ERROR)
    {
        Error("Redis command reply error, type:{},cmd:{},error:{}", pInfo->m_type, pInfo->m_lastcmd, reply->str);
    }

    switch (pInfo->m_type)
    {
    case RedisFetchCmdType::RedisCmdConnect:
    case RedisFetchCmdType::RedisCmdAuth:
        pInfo->m_server->OnAuth(reply);
        break;
    case RedisFetchCmdType::RedisCmdSelect:
        pInfo->m_server->OnSelect(reply);
        break;
    case RedisFetchCmdType::RedisCmdRpushList:
        pInfo->m_server->OnRpushList(pInfo, reply);
        break;
    case RedisFetchCmdType::RedisCmdLpopList:
        // pInfo->m_server->OnLpopList(pInfo, reply);
        break;
    default:
        Warn("invalid RedisFetchCmdType, type={}", pInfo->m_type);
        break;
    }

    delete pInfo;
    pInfo = nullptr;
}

CRedisClient::CRedisClient() : m_base(nullptr), m_ctx(nullptr), m_bConnected(false), m_pServer(nullptr)
{
}
CRedisClient::~CRedisClient()
{
    if (m_bConnected)
    {
        Disconnect();
    }
    else if (m_ctx != nullptr)
    {
        redisAsyncFree(m_ctx);
        m_ctx = nullptr;
    }
}

bool CRedisClient::Init(struct event_base *base, CRedisMqServer *pServer)
{
    m_base = base;
    m_pServer = pServer;
    return true;
}

bool CRedisClient::AsyncConnect(const std::string &ip, u_short port, size_t connect_timeout)
{
    redisOptions options;
    memset(&options, 0, sizeof(options));
    REDIS_OPTIONS_SET_TCP(&options, ip.c_str(), port);
    struct timeval tv = {(time_t)connect_timeout, 0};
    options.connect_timeout = &tv;
    m_ctx = redisAsyncConnectWithOptions(&options);
    if (m_ctx == nullptr)
    {
        Error("CRedisClient::Init connect to redis {}:{} failed, error:{}", ip, port, m_ctx->errstr);
        return false;
    }

    m_ip = ip;
    m_port = port;

    m_ctx->data = m_pServer;
    m_ctx->dataCleanup = nullptr; // 不需要通过函数来清理该指针，调用清理函数时，肯定在析构函数中

    if (REDIS_OK != redisLibeventAttach(m_ctx, m_base))
    {
        Error("CRedisClient redisLibeventAttach failed");
        return false;
    }
    if (REDIS_OK != redisAsyncSetConnectCallback(m_ctx, RedisConnectCallback))
    {
        Error("CRedisClient redisAsyncSetConnectCallback failed");
        return false;
    }
    if (REDIS_OK != redisAsyncSetDisconnectCallback(m_ctx, RedisDisconnectCallback))
    {
        Error("CRedisClient redisAsyncSetDisconnectCallback failed");
        return false;
    }
    return true;
}

void CRedisClient::Disconnect()
{
    if (m_bConnected)
    {
        if (m_ctx != nullptr)
        {
            redisAsyncDisconnect(m_ctx);
            m_ctx = nullptr;
        }
        m_bConnected = false;
    }
}
bool CRedisClient::Init_And_Send_SRedisFetchInfo(const char *buf, int n, RedisFetchCmdType type)
{
    SRedisFetchInfo *pInfo = new SRedisFetchInfo(this);
    pInfo->m_lastcmd.assign(buf, n);
    pInfo->m_type = type;

    if (redisAsyncCommand(m_ctx, RedisCommandCallback, pInfo, buf) != REDIS_OK)
    {
        Error("redisAsyncCommand failed, type:{},cmd:{}", pInfo->m_type, pInfo->m_lastcmd);
        delete pInfo;
        return false;
    }
    return true;
}

// 登录认证
bool CRedisClient::Auth(const std::string &passport)
{
    if (passport.empty())
    {
        Error("redis passport empty!");
        return false;
    }
    char buf[32];
    int n = snprintf(buf, sizeof(buf) - 1, "auth %s", passport.c_str());
    buf[n] = '\0';
    return Init_And_Send_SRedisFetchInfo(buf, n, RedisFetchCmdType::RedisCmdAuth);
}
// 选择指定数据库
bool CRedisClient::Select(size_t index)
{
    char buf[10];
    int n = snprintf(buf, sizeof(buf) - 1, "select %zu", index);
    buf[n] = '\0';
    return Init_And_Send_SRedisFetchInfo(buf, n, RedisFetchCmdType::RedisCmdSelect);
}
// 创建消息组，目前是从最新的开始读，不会读取历史数据
bool CRedisClient::CreateGroup(const std::string &stream, const std::string &group)
{
    char buf[64];
    int n = snprintf(buf, sizeof(buf) - 1, "xgroup create %s %s $ MKSTREAM", stream.c_str(), group.c_str());
    buf[n] = '\0';
    return Init_And_Send_SRedisFetchInfo(buf, n, RedisFetchCmdType::RedisCmdXgroupCreate);
}

bool CRedisClient::DestroyGroup(const std::string &stream, const std::string &group)
{
    char buf[64];
    int n = snprintf(buf, sizeof(buf) - 1, "xgroup destroy %s %s", stream.c_str(), group.c_str());
    buf[n] = '\0';
    return Init_And_Send_SRedisFetchInfo(buf, n, RedisFetchCmdType::RedisCmdXGroupDestroy);
}

bool CRedisClient::XreadGroup(const std::string &stream, const std::string &group, const std::string &consumer, int count, int block_timeout)
{
    char buf[128];
    int n = 0;
    if (block_timeout >= 0)
    {
        n = snprintf(buf, sizeof(buf) - 1, "xreadgroup group %s %s count %d block %d streams %s >",
                     group.c_str(), consumer.c_str(), count, block_timeout, stream.c_str());
    }
    else
    {
        n = snprintf(buf, sizeof(buf) - 1, "xreadgroup group %s %s count %d streams %s >",
                     group.c_str(), consumer.c_str(), count, stream.c_str());
    }
    return Init_And_Send_SRedisFetchInfo(buf, n, RedisFetchCmdType::RedisCmdXreadGroup);
}

bool CRedisClient::RpushList(const std::string &list, const std::string &args)
{
    char buf[2048 + 512]; // 注意redis的长度限制，并且args中不能有空格。避免获取数据太少
    int n = snprintf(buf, sizeof(buf) - 1, "rpush %s %s", list.c_str(), args.c_str());
    buf[n] = '\0';
    return Init_And_Send_SRedisFetchInfo(buf, n, RedisFetchCmdType::RedisCmdRpushList);
}

bool CRedisClient::LpopList(const std::string &list)
{
    char buf[108];
    int n = snprintf(buf, sizeof(buf) - 1, "lpop %s", list.c_str());
    buf[n] = '\0';
    return Init_And_Send_SRedisFetchInfo(buf, n, RedisFetchCmdType::RedisCmdLpopList);
}
