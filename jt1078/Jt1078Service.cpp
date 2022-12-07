#include "Jt1078Service.h"
#include "../core/log/Log.hpp"
#include "../core/ini_config.h"
#include "../3part/nlohmann/json.hpp"
#include "../core/socket/Socket.h"

Jt1078Service::Jt1078Service() : m_is_history_server(false)
{
}
Jt1078Service::~Jt1078Service()
{
    // 最后视频m_eventloop，先释放其他依赖组件
    m_jt1078_server.reset();
    m_forward_server.reset();
    m_redis_server.reset();
}

bool Jt1078Service::Init()
{
    if (!m_eventloop.Init())
    {
        Error("Jt1078Service::Init failed, eventloop init failed");
        return false;
    }

    m_is_history_server = g_ini->GetBoolean("jt1078", "is_history_server", false);
    m_redis_808_list = g_ini->Get("redis", "808_command_list", "");
    if (m_redis_808_list.empty())
    {
        Error("Jt1078Service::Init failed, config 808_command_list empty");
        return false;
    }

    m_jt1078_server = std::make_unique<Jt1078Server>(&m_eventloop, this);
    const std::string jt1078_server_domain = g_ini->Get("jt1078", "domain", "");
    m_jt1078_server->SetServerDomain(jt1078_server_domain);
    const std::string jt1078_listen_ip = g_ini->Get("jt1078", "ip", "");
    u_short jt1078_listen_port = g_ini->GetInteger("jt1078", "port", INVALID_PORT);
    if (!m_jt1078_server->Listen(jt1078_listen_ip, jt1078_listen_port))
    {
        Error("Jt1078Service::Init failed, jt1078_server listen failed");
        return false;
    }

    m_forward_server = std::make_unique<forward::ForwardServer>(&m_eventloop, this);
    const std::string forward_listen_ip = g_ini->Get("forward", "ip", "");
    u_short forward_listen_port = g_ini->GetInteger("forward", "port", 0);
    if (!m_forward_server->Listen(forward_listen_ip, forward_listen_port))
    {
        Error("Jt1078Service::Init failed, forward_server listen failed");
        return false;
    }

    m_redis_server = std::make_unique<CRedisMqServer>();
    if (!m_redis_server->Init(&m_eventloop))
    {
        Error("Jt1078Service::Init failed, redis server init failed");
        return false;
    }

    return true;
}
bool Jt1078Service::Start()
{
    m_eventloop.Loop();
    return true;
}

void Jt1078Service::Notify1078Packet(device_id_t device_id ,const jt1078::packet_t &pkt)
{
    m_forward_server->Publish(device_id, pkt);
}

void Jt1078Service::SendCommandTo808(const std::string &strDeviceId, bool bConnect)
{
    if (strDeviceId.empty() || strDeviceId.size() != 14)
    {
        Error("Jt1078Service::SendCommandTo808 failed, invalid strDeviceId:{},len:{},bConnected:{}", strDeviceId, strDeviceId.size(), bConnect);
        return;
    }

    const std::string strSim = strDeviceId.substr(0, 12);
    std::string strLogicNumber = strDeviceId.substr(12, 2);
    if (strLogicNumber[0] == '0')
    {
        strLogicNumber[0] = strLogicNumber[1];
        strLogicNumber.resize(1);
    }
    const std::string &domain = m_jt1078_server->GetServerDomain();
    const std::string port = std::to_string(m_jt1078_server->GetListenPort());

    std::string strJson;
    try
    {
        // 要求字段值全是字符串类型
        if (m_is_history_server) // 是历史服务器
        {
            nlohmann::json j{
                //{"port", std::to_string(m_uListenPort)},
                //{"ip", m_strMyHost},
                {"iccid", strSim},
                {"channelNo", strLogicNumber},
                // {"dataType", "0"},       // 0表示音视频
                //{"codeStreamType", "1"}, // 0表示主码流，1表示子码流
                {"commandId", "9202"}}; // 9202表示关闭录像
            strJson = j.dump();
        }
        else // 是实时视频服务器
        {
            if (bConnect)
            {
                nlohmann::json j{
                    {"port", port},
                    {"ip", domain},
                    {"iccid", strSim},
                    {"channelNo", strLogicNumber},
                    {"dataType", "0"},       // 0表示音视频
                    {"codeStreamType", "1"}, // 0表示主码流，1表示子码流
                    {"commandId", "9101"}};  // 9101表示打开音频流，9102关闭音频流
                strJson = j.dump();
            }
            else
            {
                nlohmann::json j{
                    {"port", port},
                    {"ip", domain},
                    {"iccid", strSim},
                    {"channelNo", strLogicNumber},
                    {"dataType", "0"},       // 0表示音视频
                    {"codeStreamType", "1"}, // 0表示主码流，1表示子码流
                    {"commandId", "9102"}};  // 9101表示打开音频流，9102关闭音频流
                strJson = j.dump();
            }
        }
    }
    catch (const std::exception &e)
    {
        Error("Jt1078Service::SendCommandTo808, get a json exception:{}", e.what());
        return;
    }
    m_redis_server->RpushList(m_redis_808_list, strJson);
}