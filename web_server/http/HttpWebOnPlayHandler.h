#ifndef HTTP_WEB_ON_PLAY_HANDLER_H
#define HTTP_WEB_ON_PLAY_HANDLER_H
#include "../../core/http_server/HttpHandler.h"

class WebServer;
class HttpWebOnPlayHandler : public HttpHandler
{
public:
    HttpWebOnPlayHandler(WebServer *server);
    virtual ~HttpWebOnPlayHandler() override {}

protected:
    virtual void HandlerRequest(struct evhttp_request *request) override;

private:
    WebServer *GetHttpServer();

    void HandlerJsonData(struct evhttp_request *request, const nlohmann::json &j, const std::string &strJsonData);
    bool GetDeviceIdFromJson(const nlohmann::json &j, std::string &strDeviceId);
};

#endif // HTTP_WEB_ON_PLAY_HANDLER_H