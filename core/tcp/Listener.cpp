#include "./Listener.h"
#include "../eventloop/EventLoop.h"
#include "../socket/Socket.h"
#include <event2/listener.h>
#include <string.h>
#include "../log/Log.hpp"

Listener::Listener(EventLoop *eventloop) : m_eventloop(eventloop),
                                           m_port(0),
                                           m_listen_socket(INVALID_SOCKET),
                                           m_listener(nullptr)
{
}
Listener::~Listener()
{
    Clear();
}
void Listener::Clear()
{
    m_eventloop = nullptr;
    m_ip.clear();
    m_port = INVALID_SOCKET;
    if (m_listener)
    {
        evconnlistener_free(m_listener);
        m_listener = nullptr;
    }
    m_connection_callback = nullptr;
}

bool Listener::Listen(const std::string &ip, u_short port)
{
    m_ip = ip;
    m_port = port;
    m_listen_socket = sock::GetTcpListenSocket();
    if (m_listen_socket == INVALID_SOCKET)
    {
        Error("Listener::Listen, GetTcpListenSocket failed,ip:{},port:{}", ip, port);
        return false;
    }
    struct sockaddr_in addr;
    if (!sock::SetSocketIpv4Addr(m_ip, m_port, addr))
    {
        Error("Listener::Listen, SetSockaddr failed, ip:{},port:{}", m_ip, m_port);
        return false;
    }

    int nerrno = 0;
    m_listener = evconnlistener_new_bind(m_eventloop->GetEventBase(), Listener::listener_cb, this, LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE,
                                         -1, (struct sockaddr *)&addr, sizeof(addr));
    nerrno = errno;
    if (m_listener == nullptr)
    {
        Error("Listener::Listen, evconnlistener_new_bind failed, fd:{},ip:{},port:{}, error:{}",
              m_listen_socket, m_ip.c_str(), m_port, strerror(nerrno));
        evutil_closesocket(m_listen_socket);
        m_listen_socket = INVALID_SOCKET;
        return false;
    }
    return true;
}

void Listener::listener_cb(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *sa, int socklen, void *arg)
{
    Listener *obj = static_cast<Listener *>(arg);
    obj->NewConnection(fd, sa);
}
void Listener::NewConnection(evutil_socket_t socket, struct sockaddr *sa)
{
    if (m_connection_callback)
    {
        m_connection_callback(socket, sa);
    }
}
