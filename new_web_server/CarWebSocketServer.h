#ifndef CAR_WEBSOCKET_SERVER_H
#define CAR_WEBSOCKET_SERVER_H

#include <map>
#include "WebSocketServer.h"
#include "../jt1078/AV_Common_Define.h"

class CarTopicManager;

class CarWebSocketServer : public WebSocketServer
{
    // 处理订阅车辆信息的回调函数，bSubscribe=true表示订阅，=false表示取消订阅
    typedef std::function<void(const std::string &strDeviceId, bool bSubscribe)> FuncNotifySubscribeCarDeviceId;

public:
    CarWebSocketServer(asio::io_service &io_service, const asio::ip::tcp::endpoint &endpoint);
    virtual ~CarWebSocketServer() {}

public:
    void SetFuncSubscribeCarDeviceId(FuncNotifySubscribeCarDeviceId func) { m_func_notify_subscribe = func; }

    void UpdateData(device_id_t device_id, const char *data, size_t length);
    void SendToHdl(websocketpp::connection_hdl hdl, const char *data, size_t lenght);

protected:
    virtual void onOpen(websocketpp::connection_hdl hdl) override;
    virtual void onMessage(websocketpp::connection_hdl hdl, websocketpp::server<websocketpp::config::asio>::message_ptr msg) override;
    virtual void onClose(websocketpp::connection_hdl hdl) override;
    virtual void onError(websocketpp::connection_hdl hdl) override;

private:
    int AddHdlAndSubscribeCarDeviceId(websocketpp::connection_hdl hdl, const std::string &strDeviceId);
    // 释放hdl资源，并且取消订阅相关话题
    void RemoveHdlAndUnSubscribeCarDeviceId(websocketpp::connection_hdl hdl);

private:
    std::shared_ptr<CarTopicManager> m_topic_manager;
    std::map<websocketpp::connection_hdl, std::string, std::owner_less<websocketpp::connection_hdl>> m_subscribers; // 保存订阅列表，key是hdl，value是deviceId

    FuncNotifySubscribeCarDeviceId m_func_notify_subscribe;
};

#endif // CAR_WEBSOCKET_SERVER_H