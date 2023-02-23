#include "CarWebServer.h"

// 处理jt1078数据包
void CarWebServer::HandlerJt1078Data(device_id_t device_id, const char *data, size_t length)
{
    m_websocket_server->UpdateData(device_id, data, length);
}

void CarWebServer::HandlerWsConnSubscribe(const std::string &strDeviceId, bool bSubscribe)
{
    if (m_tcp_client)
    {
        if (bSubscribe)
        {
            m_tcp_client->SendSubscribeRequest(strDeviceId);
        }
        else
        {
            m_tcp_client->SendUnSubscribeRequest(strDeviceId);
        }
    }
}
