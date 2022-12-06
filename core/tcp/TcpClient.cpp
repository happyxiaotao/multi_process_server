#include "TcpClient.h"
#include "../socket/Socket.h"
#include "../log/Log.hpp"
#include "../eventloop/EventLoop.h"
TcpClient::TcpClient() : m_ev_write(nullptr), m_bConnected(false)
{
    evutil_timerclear(&m_connect_timeout);
    Trace("TcpClient::TcpClient");
}
TcpClient::~TcpClient()
{
    Trace("TcpClient::~TcpClient");
    Clear();
}
bool TcpClient::AsyncConnect(EventLoop *eventloop, const std::string &ip, u_short port, struct timeval *connect_timeout)
{
    FreeWriteEvent();

    SetEventLoop(eventloop);
    SetRemoteIp(ip);
    SetRemotePort(port);

    if (connect_timeout != nullptr)
    {
        SetConnectTimeout(*connect_timeout);
    }

    struct sockaddr_in client_addr;
    sock::SetSocketIpv4Addr(ip, port, client_addr);
    int client_fd = INVALID_SOCKET;
    int addrlen = sizeof(struct sockaddr_in);
    int ret = sock::SocketConnect(&client_fd, (struct sockaddr *)&client_addr, addrlen);
    if (ret < 0) //发生错误
    {
        Error("TcpClient::AsyncConnect connect failed, ip:{},port:{}", ip, port);
        return false;
    }

    Trace("TcpClient:::AsyncConnect, remote_ip:{},remote_port:{},client_fd:{}", ip, port, client_fd);
    if (ret == 0) // 还未连接，需要等待连接
    {
        // 需要监听可写事件，当可写时，表示连接成功
        struct event *event = event_new(eventloop->GetEventBase(), client_fd, EV_WRITE /*| EV_PERSIST*/, &TcpClient::tcp_session_write_cb, this);
        if (event == nullptr)
        {
            Error("TcpClient::AsyncConnect failed, write event init failed,ip:{},port:{},client_fd:{}", ip, port, client_fd);
            evutil_closesocket(client_fd);
            return false;
        }
        m_ev_write = event;
        event_add(m_ev_write, connect_timeout); // connect_timeout是连接超时时间
    }
    else if (ret == 1) // 已经连接了
    {
        const bool bConnected = true;
        OnConnected(client_fd, bConnected, "already connected");
    }
    else // ret == 2 refused
    {
        Error("TcpClient::AsyncConnect failed, refused, ip:{},port:{}", ip, port);
        const bool bConnected = false;
        OnConnected(client_fd, bConnected, "connect refused");
    }
    return true;
}
void TcpClient::FreeWriteEvent()
{
    if (m_ev_write)
    {
        event_free(m_ev_write);
        m_ev_write = nullptr;
    }
}
void TcpClient::tcp_session_write_cb(evutil_socket_t socket, short events, void *args)
{
    Trace("TcpClient::tcp_session_write_cb, event=0x{:x}", events);
    TcpClient *client = static_cast<TcpClient *>(args);
    if ((events & EV_WRITE) == EV_WRITE)
    {
        int err;
        socklen_t len = sizeof(err);
        getsockopt(socket, SOL_SOCKET, SO_ERROR, &err, &len);
        if (err)
        {
            Error("connect return ev_write, but check failed, err:{}", err);
            evutil_closesocket(socket);
            client->OnConnected(socket, false, strerror(err));
        }
        else
        {
            client->SetConnected(true);
            client->OnConnected(socket, true, "connect ok");
        }
    }
    else // timeout
    {
        client->OnConnected(socket, false, "connect timeout");
    }
}
void TcpClient::Clear()
{
    FreeWriteEvent();
    SetConnected(false);
    evutil_timerclear(&m_connect_timeout);
    TcpSession::Clear();
}