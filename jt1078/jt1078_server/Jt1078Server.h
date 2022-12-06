#ifndef JT1078_SERVER_H
#define JT1078_SERVER_H

#include "../../core/tcp/Listener.h"
#include "CarManager.h"
class Jt1078Service;
class Jt1078Server
{
public:
    Jt1078Server(EventLoop *eventloop, Jt1078Service *service);
    ~Jt1078Server();

public:
    int Listen(const std::string &ip, u_short port);

    inline void SetServerDomain(const std::string &domain) { m_server_domain = domain; }
    inline const std::string &GetServerDomain() const { return m_server_domain; }
    inline const std::string &GetListenIp() const { return m_listen_ip; }
    inline u_short GetListenPort() const { return m_listen_port; }

private:
    void OnNewConnection(evutil_socket_t socket, struct sockaddr *sa);

    void OnPacketError(const CarSessionPtr &car, TcpErrorType error_type);
    void OnPacketCompleted(const CarSessionPtr &car, const jt1078::packet_t &pkt);

private:
    std::string m_server_domain; //服务器的域名
    std::string m_listen_ip;     //服务器监听的ip
    u_short m_listen_port;       // 服务器监听的端口
    EventLoop *m_eventloop;
    ListenerPtr m_listener; // 接收jt1078数据包
    CarManager m_manager;
    Jt1078Service *m_service;
};

#endif // JT1078_SERVER_H