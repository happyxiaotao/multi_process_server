#ifndef HTTP_HANDLER_H
#define HTTP_HANDLER_H
#include <event2/http.h>
#include <event2/http_struct.h>
#include <string>
#include <nlohmann/json.hpp>
class HttpServer;
class HttpHandler
{
public:
    HttpHandler(HttpServer *server) : m_server(server)
    {
    }
    virtual ~HttpHandler() {}

public:
    bool SetUri(const std::string &uri);

protected:
    inline HttpServer *GetHttpServer() const { return m_server; }
    inline const std::string &GetUri() const { return m_uri; }

    virtual void HandlerRequest(struct evhttp_request *request) = 0;

    // 获取服务器指针
    HttpServer *GetHttpServer() { return m_server; }

    void SendReply_BadMethod(struct evhttp_request *request, const char *reason = nullptr);
    void SendReply_BadRequest(struct evhttp_request *request, const char *reason = nullptr);
    void SendReply_OK(evhttp_request *request, const char *data = nullptr, size_t len = 0);

protected:
    // 根据cmd_type获取对应字符串
    static const char *GetHttpCmdTypeStr(evhttp_cmd_type cmd);
    // 转化为json格式
    static bool ParseJsonData(const char *pData, size_t len, nlohmann::json &j);

private:
    static void http_callback(struct evhttp_request *request, void *args);

private:
    HttpServer *m_server;
    std::string m_uri;
};

#endif // HTTP_HANDLER_H