#ifndef SWORD_LISTENER_H
#define SWORD_LISTENER_H

#include <event2/event.h>
#include <functional>
#include <string>
#include <memory>
struct evconnlistener;
class EventLoop;
class Listener;
typedef std::shared_ptr<Listener> ListenerPtr;
class Listener
{
public:
    typedef std::function<void(evutil_socket_t socket, struct sockaddr *sa)> ConnectionCallBack;

public:
    Listener(EventLoop *eventloop);
    ~Listener();

public:
    void SetConnectionCallBack(const ConnectionCallBack &cb) { m_connection_callback = cb; }
    void Clear();
    bool Listen(const std::string &ip, u_short port);

private:
    static void listener_cb(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *sa, int socklen, void *arg);
    void NewConnection(evutil_socket_t socket, struct sockaddr *sa);

private:
    EventLoop *m_eventloop;
    std::string m_ip;
    u_short m_port;
    evutil_socket_t m_listen_socket; // 监听fd
    struct evconnlistener *m_listener;
    ConnectionCallBack m_connection_callback;
};

#endif // SWORD_LISTENER_H