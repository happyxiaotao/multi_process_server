
#include <event2/buffer.h>
#include "HttpHandler.h"
#include "HttpServer.h"
bool HttpHandler::SetUri(const std::string &uri)
{
    m_uri = uri;
    return evhttp_set_cb(m_server->GetEvHttp(), m_uri.c_str(), http_callback, this) == 0;
}

void HttpHandler::http_callback(struct evhttp_request *request, void *args)
{
    HttpHandler *handler = static_cast<HttpHandler *>(args);
    handler->HandlerRequest(request);
}

const char *HttpHandler::GetHttpCmdTypeStr(evhttp_cmd_type cmd)
{
    const char *cmdtype = NULL;
    switch (cmd)
    {
    case EVHTTP_REQ_GET:
        cmdtype = "GET";
        break;
    case EVHTTP_REQ_POST:
        cmdtype = "POST";
        break;
    case EVHTTP_REQ_HEAD:
        cmdtype = "HEAD";
        break;
    case EVHTTP_REQ_PUT:
        cmdtype = "PUT";
        break;
    case EVHTTP_REQ_DELETE:
        cmdtype = "DELETE";
        break;
    case EVHTTP_REQ_OPTIONS:
        cmdtype = "OPTIONS";
        break;
    case EVHTTP_REQ_TRACE:
        cmdtype = "TRACE";
        break;
    case EVHTTP_REQ_CONNECT:
        cmdtype = "CONNECT";
        break;
    case EVHTTP_REQ_PATCH:
        cmdtype = "PATCH";
        break;
    default:
        cmdtype = "unknown";
        break;
    }
    return cmdtype;
}

void HttpHandler::SendReply_BadMethod(evhttp_request *request, const char *reason)
{
    evhttp_send_error(request, HTTP_BADMETHOD, reason);
}

void HttpHandler::SendReply_BadRequest(evhttp_request *request, const char *reason)
{
    evhttp_send_error(request, HTTP_BADREQUEST, reason);
}

void HttpHandler::SendReply_OK(evhttp_request *request, const char *data, size_t len)
{
    struct evbuffer *buffer = evbuffer_new();
    if (data != nullptr && len > 0)
    {
        evbuffer_add(buffer, data, len);
    }
    evhttp_send_reply(request, HTTP_OK, nullptr, buffer);
    evbuffer_free(buffer);
}

bool HttpHandler::ParseJsonData(const char *pData, size_t len, nlohmann::json &j)
{
    j.clear();
    try
    {
        j = nlohmann::json::parse(pData, pData + len);
    }
    catch (const std::exception &e)
    {
        return false;
    }
    return true;
}
