#ifndef HTTP_SRS_ON_SESSION_HANDLER_H
#define HTTP_SRS_ON_SESSION_HANDLER_H
#include "../../core/http_server/HttpHandler.h"
#include <nlohmann/json.hpp>
#include <set>

class WebServer;

// 统计浏览器发送的所有clientId.
class HttpSessionStat
{
public:
    HttpSessionStat() {}
    ~HttpSessionStat() {}

public:
    size_t Size(const std::string &strSession)
    {
        return m_mapClients[strSession].size();
    }
    bool EmptyClientId(const std::string &strSession)
    {
        return Size(strSession) == 0;
    }
    void AddClientId(const std::string &strStream, const std::string &strClientId)
    {
        m_mapClients[strStream].insert(strClientId);
    }
    void DelClientId(const std::string &strStream, const std::string &strClientId)
    {
        m_mapClients[strStream].erase(strClientId);
    }
    void ReleaseStream(const std::string &strStream)
    {
        m_mapClients.erase(strStream);
    }

private:
    std::map<std::string, std::set<std::string>> m_mapClients;
};

class HttpSrsOnSessionHandler : public HttpHandler
{
public:
    HttpSrsOnSessionHandler(WebServer *server);
    virtual ~HttpSrsOnSessionHandler() override {}

protected:
    virtual void HandlerRequest(struct evhttp_request *request) override;

private:
    WebServer *GetHttpServer();

    bool GetActionAndStream(const nlohmann::json &j, std::string &strAction, std::string &strStream, std::string &strClientId, std::string &strApp);

    void HandlerJsonData(struct evhttp_request *request, const nlohmann::json &j, const std::string &strJsonData);

    bool SessionStop(struct evhttp_request *request, const std::string &strStream, const std::string &strClientId, const std::string &strApp);
    bool SessionPlay(struct evhttp_request *request, const std::string &strStream, const std::string &strClientId, const std::string &strApp);

    void ReturnSrsOK(struct evhttp_request *request);
    void ReturnSrsError(struct evhttp_request *request);

private:
    HttpSessionStat m_sessionStat;
};

#endif // HTTP_SRS_ON_SESSION_HANDLER_H