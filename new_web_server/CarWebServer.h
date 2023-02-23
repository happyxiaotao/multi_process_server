#ifndef CAR_WEB_SERVER_H
#define CAR_WEB_SERVER_H

#include "CarWebSocketServer.h"
#include "CarTcpClient.h"

typedef uint64_t device_id_t;

// 需要实现一个发布订阅模式
// CarWebSocketServer接收外界连接，订阅某个车辆数据
// CarTcpClient 连接jt1078Server，获取车辆视频数据
// CarWebSocketServer --> CarWebServer  : 订阅某个车辆数据
// CarTcpClient --> CarWebServer: 推送某个车辆数据
// CarWebServer --> CarWebSocketServer: 分发车辆数据

class CarWebServer
{
public:
    CarWebServer(asio::io_context &io_context) : m_io_context(io_context)
    {
    }
    ~CarWebServer() {}

    void Listen(const std::string &ip, u_short port)
    {
        if (m_websocket_server != nullptr)
        {
            m_websocket_server.reset();
        }
        auto address = asio::ip::address::from_string(ip);
        // asio::ip::tcp::endpoint endpoint(asio::ip::tcp::v4(), port);
        asio::ip::tcp::endpoint endpoint(address, port);
        m_websocket_server = std::make_unique<CarWebSocketServer>(m_io_context, endpoint);
        m_websocket_server->SetFuncSubscribeCarDeviceId(std::bind(&CarWebServer::HandlerWsConnSubscribe, this, std::placeholders::_1, std::placeholders::_2));
        m_websocket_server->Start();
    }
    void ConnectServer(const std::string &host, u_short port)
    {
        if (m_tcp_client != nullptr)
        {
            m_tcp_client.reset();
        }
        m_tcp_client = std::make_unique<CarTcpClient>(m_io_context, host, std::to_string(port));
        // 通过回调函数，实现CarTcpClient通知CarWebServer
        m_tcp_client->SetFuncNotify(std::bind(&CarWebServer::HandlerJt1078Data, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    }

    void HandlerJt1078Data(device_id_t device_id, const char *data, size_t length);
    void HandlerWsConnSubscribe(const std::string &strDeviceId, bool bSubscribe);

private:
    asio::io_context &m_io_context;
    std::shared_ptr<CarWebSocketServer> m_websocket_server;
    std::shared_ptr<CarTcpClient> m_tcp_client; // 不能使用unique_ptr，应为tcp_client继承了std::enable_shared_from_this类。否则在调用shared_from_this()时会报错崩溃
};

#endif // CAR_WEB_SERVER_H