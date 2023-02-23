#include "CarWebSocketServer.h"
#include "../core/log/Log.hpp"
#include "CarTopicManager.h"
#include "../jt1078/Jt1078Util.h"

CarWebSocketServer::CarWebSocketServer(asio::io_service &io_service, const asio::ip::tcp::endpoint &endpoint)
    : WebSocketServer(io_service, endpoint)
{
    m_topic_manager = std::make_shared<CarTopicManager>(this);
}

void CarWebSocketServer::UpdateData(device_id_t device_id, const char *data, size_t length)
{
    const bool bCreateIfNotExists = false;
    auto topic = m_topic_manager->GetTopic(device_id, bCreateIfNotExists);
    if (topic == nullptr)
    {
        return;
    }
    topic->Publish(data, length);
}

void CarWebSocketServer::SendToHdl(websocketpp::connection_hdl hdl, const char *data, size_t lenght)
{
    auto conn = GetServer().get_con_from_hdl(hdl);
    conn->send(data, lenght, websocketpp::frame::opcode::BINARY);
}

void CarWebSocketServer::onOpen(websocketpp::connection_hdl hdl)
{
    Trace("CarWebSocketServer::onOpen, get new connection_hdl");
    auto conn = GetServer().get_con_from_hdl(hdl);
    if (conn == nullptr)
    {
        return;
    }
    std::string uri = conn->get_request().get_uri();
    if (uri == "/live/04095886585502")
    {
        // 处理对应的数据
    }

    std::string strLive("/live/");
    std::string strReplay("/replay/");
    // 是直播流
    if (uri.compare(0, strLive.size(), strLive) == 0)
    {
        // 获取请求的 strDeviceId
        std::string strDeviceId = uri.substr(strLive.size());
        if (strDeviceId.length() != 14)
        {
            const std::string reason("invalid device_id");
            const websocketpp::close::status::value code = websocketpp::close::status::normal;
            CloseConnectionHdl(hdl, code, reason);
            return;
        }
        Trace("get device_id:{}", strDeviceId);
        AddHdlAndSubscribeCarDeviceId(hdl, strDeviceId);
    }
    else
    {
        const std::string reason("not support uri");
        const websocketpp::close::status::value code = websocketpp::close::status::normal;
        CloseConnectionHdl(hdl, code, reason);
        return;
    }
}

void CarWebSocketServer::onMessage(websocketpp::connection_hdl hdl, websocketpp::server<websocketpp::config::asio>::message_ptr msg)
{
    Trace("CarWebSocketServer::onMessage, get msg:{}", msg->get_payload());
    auto conn = GetServer().get_con_from_hdl(hdl);
}

void CarWebSocketServer::onClose(websocketpp::connection_hdl hdl)
{
    Trace("CarWebSocketServer::onClose");
    RemoveHdlAndUnSubscribeCarDeviceId(hdl);
}

void CarWebSocketServer::onError(websocketpp::connection_hdl hdl)
{
    Trace("CarWebSocketServer::onError");
}

int CarWebSocketServer::AddHdlAndSubscribeCarDeviceId(websocketpp::connection_hdl hdl, const std::string &strDeviceId)
{
    m_subscribers[hdl] = strDeviceId;

    device_id_t device_id = GenerateDeviceId(strDeviceId);

    // 临时添加，后续需要修改的
    // xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    std::shared_ptr<CarTopic> topic = nullptr;
    if (m_topic_manager->ExistsTopic(device_id))
    {
        const bool bCreate = false;
        topic = m_topic_manager->GetTopic(device_id, bCreate);
    }
    else
    {
        const bool bCreate = true;
        topic = m_topic_manager->GetTopic(device_id, bCreate);
        // 第一次创建该话题，则需要向jt1078_server发送订阅请求
        if (m_func_notify_subscribe && topic != nullptr)
        {
            const bool bSubscribe = true;
            m_func_notify_subscribe(strDeviceId, bSubscribe);
        }
    }
    if (topic)
    {
        topic->AddSubscriber(hdl);
    }
    else
    {
        Error("CarWebSocketServer::SubscribeCarDeviceId failed, get topic failed, device_id:{}", strDeviceId);
        return -1;
    }
    return 0;
}

void CarWebSocketServer::RemoveHdlAndUnSubscribeCarDeviceId(websocketpp::connection_hdl hdl)
{
    m_subscribers.erase(hdl);
    m_topic_manager->RemoveSubscriber(hdl);
    auto list = m_topic_manager->GetEmptyTopicList();
    m_topic_manager->RemoveTopics(list); // 清空订阅的device_id
    const bool bSubscribe = false;
    for (auto &device_id : list)
    {
        if (m_func_notify_subscribe)
        {
            std::string strDeviceId = GenerateDeviceIdStrByDeviceId(device_id);
            m_func_notify_subscribe(strDeviceId, bSubscribe);
        }
    }
}