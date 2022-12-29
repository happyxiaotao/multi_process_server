#include "HttpWebOnPlayHandler.h"
#include "WebServer.h"
#include <event2/buffer.h>
#include "../../core/log/Log.hpp"
#include "../../jt1078/Jt1078Util.h"

HttpWebOnPlayHandler::HttpWebOnPlayHandler(WebServer *server)
    : HttpHandler(dynamic_cast<HttpServer *>(server))
{
}

// 示例： {"device_id":"01395221031201"}
// 示例： {\"device_id\":\"01395221031201\"}
// 示例：curl "http://127.0.0.1:9531/web_on_play" -X POST -d  "{\"device_id\":\"01395221031201\"}"
void HttpWebOnPlayHandler::HandlerRequest(evhttp_request *request)
{
    struct evbuffer *buffer = evhttp_request_get_input_buffer(request);
    const char *uri = evhttp_request_get_uri(request);
    evhttp_cmd_type type = evhttp_request_get_command(request);
    size_t post_len = evbuffer_get_length(buffer);

    if (type != evhttp_cmd_type::EVHTTP_REQ_POST) // 方法使用的是POST
    {
        Warn("HttpWebOnPlayHandler::HandlerRequest, not a post method,uri:{},method:{}", uri, GetHttpCmdTypeStr(type));
        SendReply_BadMethod(request);
        return;
    }

    // 获取post参数
    const char *_data = reinterpret_cast<const char *>(evbuffer_pullup(buffer, post_len));
    std::string strJsonData(_data, post_len);
    evbuffer_drain(buffer, post_len);

    // 转化为json格式
    nlohmann::json j;
    bool bJsonData = ParseJsonData(strJsonData.c_str(), strJsonData.size(), j);
    // 非json格式数据
    if (!bJsonData)
    {
        Warn("HttpWebOnPlayHandler::HandlerRequest, not a valid json data. uri:{},data:{}", uri, strJsonData);
        SendReply_BadRequest(request);
        return;
    }

    // 处理json数据
    HandlerJsonData(request, j, strJsonData);
}

WebServer *HttpWebOnPlayHandler::GetHttpServer()
{
    return dynamic_cast<WebServer *>(HttpHandler::GetHttpServer());
}

void HttpWebOnPlayHandler::HandlerJsonData(evhttp_request *request, const nlohmann::json &j, const std::string &strJsonData)
{
    // 获取device_id
    std::string strDeviceId;
    if (!GetDeviceIdFromJson(j, strDeviceId))
    {
        Warn("HttpWebOnPlayHandler::HandlerJsonData, get device_id failed, json:{}", strJsonData);
        SendReply_BadRequest(request);
        return;
    }

    // 格式化校验device_id
    if (!CheckDeviceIdStrFmt(strDeviceId))
    {
        Warn("HttpWebOnPlayHandler::HandlerJsonData, check device_id fmt failed, json:{}", strJsonData);
        SendReply_BadRequest(request);
        return;
    }

    // 通知外界，获取对应数据
    GetHttpServer()->NotifyStart(strDeviceId);
    SendReply_OK(request);
}

bool HttpWebOnPlayHandler::GetDeviceIdFromJson(const nlohmann::json &j, std::string &strDeviceId)
{
    strDeviceId.clear();
    try
    {
        strDeviceId = j.at("device_id");
    }
    catch (const std::exception &e)
    {
        Warn("HttpWebOnPlayHandler::GetDeviceIdFromJson failed, exception:{}", e.what());
        return false;
    }
    return true;
}
