#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H
#include <memory>
#include "HttpHandler.h"
struct event;
class EventLoop;
class HttpServer
{
public:
    HttpServer();
    virtual ~HttpServer();

    friend HttpHandler;

public:
    bool SupportSSL(const std::string &strCertificateChain, const std::string &strPrivateKey);
    bool Init(EventLoop *eventloop, const std::string &ip, u_short port);

protected:
    inline struct evhttp *GetEvHttp() { return m_http; }
    EventLoop *GetEventLoop() { return m_eventloop; }

    // HttpHandler进行初始化，设置相关回调
    virtual bool InitHttpHandlers() { return true; }

private:
    // 创建ssl版本的bufferevent
    static struct bufferevent *ssl_bev_cb(struct event_base *base, void *arg);

private:
    EventLoop *m_eventloop;
    struct evhttp *m_http;
    struct evhttp_bound_socket *m_handle;

    bool m_bSupportSSL;
    std::string m_certfile;
};

#endif // HTTP_SERVER_H