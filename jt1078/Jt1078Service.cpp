#include "Jt1078Service.h"
#include "../core/log/Log.hpp"
#include "../core/ini_config.h"
#include "../3part/nlohmann/json.hpp"
#include "../core/socket/Socket.h"

Jt1078Service::Jt1078Service()
{
}

Jt1078Service::~Jt1078Service()
{
    // 大部分组件都依赖m_eventloop，先释放其他依赖组件，最后由m_eventloop的析构函数自动释放自己
    m_jt1078_realtime_server.reset();
    m_jt1078_history_server.reset();
    m_forward_realtime_server.reset();
    m_forward_history_server.reset();

    m_redis_server.reset();
}

bool Jt1078Service::Init()
{
    if (!m_eventloop.Init())
    {
        Error("Jt1078Service::Init failed, eventloop init failed");
        return false;
    }

    m_redis_808_list = g_ini->Get("redis", "808_command_list", "");
    if (m_redis_808_list.empty())
    {
        Error("Jt1078Service::Init failed, config 808_command_list empty");
        return false;
    }

    const bool is_realtime = true;
    const bool is_history = false;

    // 创建实时车辆处理服务
    m_jt1078_realtime_server = std::make_unique<Jt1078Server>(&m_eventloop, this, is_realtime);
    const std::string jt1078_server_domain = g_ini->Get("jt1078", "domain", "");
    m_jt1078_realtime_server->SetServerDomain(jt1078_server_domain);
    const std::string jt1078_listen_ip = g_ini->Get("jt1078", "ip", "");
    u_short jt1078_listen_port = g_ini->GetInteger("jt1078", "realtime_port", INVALID_PORT);
    if (!m_jt1078_realtime_server->Listen(jt1078_listen_ip, jt1078_listen_port))
    {
        Error("Jt1078Service::Init failed, jt1078_realtime_server listen failed, ip:{},port:{}", jt1078_listen_ip, jt1078_listen_port);
        return false;
    }

    // 创建历史车辆处理服务
    m_jt1078_history_server = std::make_unique<Jt1078Server>(&m_eventloop, this, is_history);
    m_jt1078_history_server->SetServerDomain(jt1078_server_domain);
    u_short jt1078_history_listen_port = g_ini->GetInteger("jt1078", "history_port", INVALID_PORT);
    if (!m_jt1078_history_server->Listen(jt1078_listen_ip, jt1078_history_listen_port))
    {
        Error("Jt1078Service::Init failed, jt1078_history_server listen failed, ip:{},port:{}", jt1078_listen_ip, jt1078_history_listen_port);
        return false;
    }

    // 创建实时视频转发服务
    m_forward_realtime_server = std::make_unique<forward::ForwardServer>(&m_eventloop, this, is_realtime);
    const std::string forward_listen_ip = g_ini->Get("forward", "ip", "");
    u_short forward_realtime_listen_port = g_ini->GetInteger("forward", "realtime_port", INVALID_PORT);
    if (!m_forward_realtime_server->Listen(forward_listen_ip, forward_realtime_listen_port))
    {
        Error("Jt1078Service::Init failed, forward_realtime_server listen failed,ip:{},port:{}", forward_listen_ip, forward_realtime_listen_port);
        return false;
    }

    // 创建历史视频转发服务
    m_forward_history_server = std::make_unique<forward::ForwardServer>(&m_eventloop, this, is_history);
    u_short forward_history_listen_port = g_ini->GetInteger("forward", "history_port", INVALID_PORT);
    if (!m_forward_history_server->Listen(forward_listen_ip, forward_history_listen_port))
    {
        Error("Jt1078Service::Init failed, forward_history_server listen failed,ip:{},port:{}", forward_listen_ip, forward_history_listen_port);
        return false;
    }

    // 创建Redis服务
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

void Jt1078Service::Notify1078Packet(device_id_t device_id, const jt1078::packet_t &pkt, bool bRealtime)
{
    if (bRealtime)
    {
        m_forward_realtime_server->Publish(device_id, pkt);
    }
    else
    {
        m_forward_history_server->Publish(device_id, pkt);
    }
}

void Jt1078Service::SendCommandTo808(const std::string &strDeviceId, bool bConnect, bool bRealtime)
{

    if (strDeviceId.empty() || strDeviceId.size() != 14)
    {
        Error("Jt1078Service::SendCommandTo808 failed, invalid strDeviceId:{},len:{},bConnected:{},bRealtime:{}", strDeviceId, strDeviceId.size(), bConnect, bRealtime);
        return;
    }

    const std::string strSim = strDeviceId.substr(0, 12);
    std::string strLogicNumber = strDeviceId.substr(12, 2);
    if (strLogicNumber[0] == '0')
    {
        strLogicNumber[0] = strLogicNumber[1];
        strLogicNumber.resize(1);
    }
    std::string domain = m_jt1078_realtime_server->GetServerDomain();
    std::string port = std::to_string(m_jt1078_realtime_server->GetListenPort());
    if (!bRealtime)
    {
        domain = m_jt1078_history_server->GetServerDomain();
        port = std::to_string(m_jt1078_history_server->GetListenPort());
    }

    std::string strJson;
    try
    {
        // 要求字段值全是字符串类型
        if (bRealtime) // 操作实时视频车辆
        {
            if (bConnect) // 通知808请求实时视频
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
            else // 通知808断开实时视频
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
        else // 操作历史视频车辆
        {
            if (bConnect) // 目前暂时不支持通知808请求实时视频，张总还未提供接口
            {
            }
            else // 通知808断开历史视频
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
        }
    }
    catch (const std::exception &e)
    {
        Error("Jt1078Service::SendCommandTo808, get a json exception:{}", e.what());
        return;
    }
    if (!strJson.empty())
    {
        m_redis_server->RpushList(m_redis_808_list, strJson);
    }
}

void Jt1078Service::NotifyNoSubscriber(const std::string &strDeviceId, bool bRealtime)
{
    // 没有其他订阅者时，此通道没有必要再维护。断开连接即可
    CarDisconnectCause cause = CarDisconnectCause::NoSubscriber;
    if (bRealtime)
    {
        m_jt1078_realtime_server->DisconnectCar(strDeviceId, cause);
    }
    else
    {
        m_jt1078_history_server->DisconnectCar(strDeviceId, cause);
    }
}
