#ifndef _REDIS_CLIENT_H_
#define _REDIS_CLIENT_H_

#include <hiredis/hiredis.h>
#include <hiredis/async.h>
#include <hiredis/adapters/libevent.h>
#include <string>

typedef void (*Redis_Command_Callback)(redisAsyncContext *c, void *r, void *privdate);
typedef void (*Redis_Connect_Callback)(const redisAsyncContext *c, int status);
typedef void (*Redis_Disconnect_Callback)(const redisAsyncContext *c, int status);

class CRedisMqServer;

enum RedisFetchCmdType
{
    RedisCmdConnect = 0,
    RedisCmdAuth = 1,
    RedisCmdSelect = 2,

    RedisCmdRpushList,
    RedisCmdLpopList,

    RedisCmdXgroupCreate,
    RedisCmdXGroupDestroy,
    RedisCmdXreadGroup,
};
class CRedisClient;
struct SRedisFetchInfo;

class CRedisClient
{
public:
    CRedisClient();
    ~CRedisClient();

public:
    bool Init(struct event_base *base, CRedisMqServer *pServer);

    bool IsConnected() { return m_bConnected; }

    bool AsyncConnect(const std::string &ip, u_short port, size_t connect_timeout);

    // 登录认证
    bool Auth(const std::string &passport);
    // 选择指定数据库
    bool Select(size_t index);
    // 创建消息组
    bool CreateGroup(const std::string &stream, const std::string &group);
    bool DestroyGroup(const std::string &stream, const std::string &group);
    // 消费者从消费组中取消息
    bool XreadGroup(const std::string &stream, const std::string &group, const std::string &consumer, int count = 1, int block_timeout = -1);

    //向队列中推送数据
    bool RpushList(const std::string &list, const std::string &args);

    // 从队列中取数据
    bool LpopList(const std::string &list);

    void SetConnected(bool flag)
    {
        // if (!flag)
        // {
        //     Disconnect();
        // }
        m_bConnected = flag;
    }

    friend struct SRedisFetchInfo;

private:
    void Disconnect();

    bool Init_And_Send_SRedisFetchInfo(const char *buf, int n, RedisFetchCmdType type);

private:
    struct event_base *m_base;
    redisAsyncContext *m_ctx;

    std::string m_ip;
    u_short m_port;
    std::string m_passport;
    size_t m_index;

    std::string m_stream;   // 消息队列 流名称
    std::string m_group;    // 消息队列 组名称
    std::string m_consumer; // 消息队列 消费者名称

    bool m_bConnected;

    CRedisMqServer *m_pServer;
};
struct SRedisFetchInfo
{
    SRedisFetchInfo(CRedisClient *client) : m_client(client) { m_server = m_client->m_pServer; }
    CRedisClient *m_client;
    CRedisMqServer *m_server;
    RedisFetchCmdType m_type;
    std::string m_lastcmd;
};

#endif //_REDIS_CLIENT_H_