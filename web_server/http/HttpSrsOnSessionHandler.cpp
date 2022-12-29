#include "HttpSrsOnSessionHandler.h"
#include "WebServer.h"
#include <event2/buffer.h>
#include "../../core/log/Log.hpp"
#include "../../jt1078/Jt1078Util.h"

HttpSrsOnSessionHandler::HttpSrsOnSessionHandler(WebServer *server)
    : HttpHandler(dynamic_cast<HttpServer *>(server))
{
}

// 测试url:{\"client_id\":\"dsfsdfsafd\",\"stream\":\"01395221031201\",\"action\":\"on_play\"}
// 测试url:{\"client_id\":\"dsfsdfsafd\",\"stream\":\"01395221031201\",\"action\":\"on_stop\"}
// curl "http://127.0.0.1:9531/api/v1/sessions" -X POST -d "{\"client_id\":\"dsfsdfsafd\",\"stream\":\"01395221031201\",\"action\":\"on_play\"}"
// curl "http://127.0.0.1:9531/api/v1/sessions" -X POST -d "{\"client_id\":\"dsfsdfsafd\",\"stream\":\"01395221031201\",\"action\":\"on_stop\"}"
void HttpSrsOnSessionHandler::HandlerRequest(evhttp_request *request)
{
    struct evbuffer *buffer = evhttp_request_get_input_buffer(request);
    const char *uri = evhttp_request_get_uri(request);
    evhttp_cmd_type type = evhttp_request_get_command(request);
    size_t post_len = evbuffer_get_length(buffer);

    if (type != evhttp_cmd_type::EVHTTP_REQ_POST) // 方法使用的是POST
    {
        Warn("HttpSrsOnSessionHandler::HandlerRequest, not a post method,uri:{},method:{}", uri, GetHttpCmdTypeStr(type));
        SendReply_BadMethod(request);
        return;
    }

    if (post_len == 0) // 正常情况下，post参数长度不为0
    {
        Warn("HttpSrsOnSessionHandler::HandlerRequest, post_len==0,uri:{},method:{}", uri, GetHttpCmdTypeStr(type));
        SendReply_BadRequest(request);
        return;
    }

    // 获取post参数
    const char *_data = reinterpret_cast<const char *>(evbuffer_pullup(buffer, post_len));
    std::string strJsonData(_data, post_len);
    evbuffer_drain(buffer, post_len);

    // 处理data
    nlohmann::json j;
    bool bJsonData = ParseJsonData(strJsonData.c_str(), strJsonData.size(), j);
    // 非json格式数据
    if (!bJsonData)
    {
        Warn("HttpSrsOnSessionHandler::HandlerRequest, not a valid json data. uri:{},data:{}", uri, strJsonData);
        SendReply_BadRequest(request);
        return;
    }

    // 处理json数据
    HandlerJsonData(request, j, strJsonData);
}

WebServer *HttpSrsOnSessionHandler::GetHttpServer()
{
    return dynamic_cast<WebServer *>(HttpHandler::GetHttpServer());
}

bool HttpSrsOnSessionHandler::GetActionAndStream(const nlohmann::json &j, std::string &strAction, std::string &strStream, std::string &strClientId)
{
    // 存在不同SRS版本，回调的client_id字段类型不一样的情况。SRS3中是number，SRS4中是字符串
    strAction.clear();
    strStream.clear();
    strClientId.clear();
    try
    {
        strAction = j.at("action");
        strStream = j.at("stream");
        // 针对SRS3中为number，SRS4中为string的情况，进行特殊处理
        auto &json_client_id = j["client_id"];
        if (json_client_id.is_number())
        {
            int nClientId = json_client_id.get<int>();
            strClientId = std::to_string(nClientId);
        }
        else if (json_client_id.is_string())
        {
            strClientId = json_client_id.get<std::string>();
        }
        else
        {
            Warn("HttpSrsOnSessionHandler::GetActionAndStream, client_id not a number or string type");
            return false;
        }
    }
    // catch (const nlohmann::json::parse_error &e) // 使用nlohmann::json::parse_error有时候捕获不到一些异常。使用std::exception获取就可以捕获异常
    catch (const std::exception &e)
    {
        Warn("HttpSrsOnSessionHandler::GetActionAndStream, get json data 'action' or'stream' failed, error:{}", e.what());
        return false;
    }
    return true;
}

void HttpSrsOnSessionHandler::HandlerJsonData(struct evhttp_request *request, const nlohmann::json &j, const std::string &strJsonData)
{
    std::string strAction;
    std::string strStream;
    std::string strClientId;
    if (!GetActionAndStream(j, strAction, strStream, strClientId))
    {
        Warn("HttpSrsOnSessionHandler::HandlerJsonData, GetActionAndStream failed!,json data:{}", strJsonData);
        SendReply_BadRequest(request);
        return;
    }
    if (strStream.empty() || (strAction != "on_stop" && strAction != "on_play"))
    {
        Warn("HttpSrsOnSessionHandler::HandlerJsonData, get invalid session callback json data:{}", strJsonData);
        SendReply_BadRequest(request);
        return;
    }

    // 校验，是否合理，符合device_id的规格
    if (!CheckDeviceIdStrFmt(strStream))
    {
        Warn("HttpSrsOnSessionHandler::HandlerJsonData, check device id fmt failed, json data:{}", strJsonData);
        ReturnSrsError(request);
        return;
    }

    bool b = true;
    if (strAction == "on_stop")
    {
        b = SessionStop(request, strStream, strClientId);
    }
    else if (strAction == "on_play") // on_play
    {
        b = SessionPlay(request, strStream, strClientId);
    }
    // 处理成功
    if (b)
    {
        ReturnSrsOK(request);
    }
    else
    {
        ReturnSrsError(request);
    }
}

bool HttpSrsOnSessionHandler::SessionStop(evhttp_request *request, const std::string &strStream, const std::string &strClientId)
{
    Info("HttpSrsOnSessionHandler,on_stop,stream={},clientid={}", strStream, strClientId);
    // 通知web_server，断开某个会话
    m_sessionStat.DelClientId(strStream, strClientId);
    if (m_sessionStat.EmptyClientId(strStream)) // 当没有浏览器在观看此通道后，通知关闭此通道
    {
        GetHttpServer()->NotifyStop(strStream);

        // 释放此通道的缓存
        m_sessionStat.ReleaseStream(strStream);
    }
    return true;
}

bool HttpSrsOnSessionHandler::SessionPlay(evhttp_request *request, const std::string &strStream, const std::string &strClientId)
{
    Info("HttpSrsOnSessionHandler,on_play,stream={},clientid={}", strStream, strClientId);

    // 如果没有其他浏览器观看此视频，表示第一次观看，需要通道web_server进行订阅
    if (m_sessionStat.EmptyClientId(strStream))
    {
        GetHttpServer()->NotifyStart(strStream);
    }

    m_sessionStat.AddClientId(strStream, strClientId);
    return true;
}

void HttpSrsOnSessionHandler::ReturnSrsOK(evhttp_request *request)
{
    SendReply_OK(request, "0", 1);
}

void HttpSrsOnSessionHandler::ReturnSrsError(evhttp_request *request)
{
    SendReply_OK(request, "1", 1);
}
