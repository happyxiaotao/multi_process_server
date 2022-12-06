#ifndef COER_TCP_CLIENT_H
#define COER_TCP_CLIENT_H
#include "TcpSession.h"
class TcpClient;
typedef std::shared_ptr<TcpClient> TcpClientPtr;
class TcpClient : public TcpSession
{
public:
    TcpClient();
    virtual ~TcpClient() override;

public:
    bool AsyncConnect(EventLoop *eventloop, const std::string &ip, u_short port, struct timeval *connect_timeout);

    inline void SetConnectTimeout(struct timeval &timeout) { memcpy(&m_connect_timeout, &timeout, sizeof(timeout)); }
    inline const struct timeval &GetConnectTimeout() const { return m_connect_timeout; }
    inline bool IsConnected() { return m_bConnected; }

    virtual void Clear();

protected:
    virtual void OnConnected(evutil_socket_t fd, bool bOk, const char *err) { SetConnected(bOk); }
    virtual void OnError(const TcpSessionPtr &tcp, TcpErrorType error_type) override { SetConnected(false); }
    inline void SetConnected(bool flag) { m_bConnected = flag; }

private:
    static void tcp_session_write_cb(evutil_socket_t socket, short events, void *args);
    void FreeWriteEvent();

private:
    // 添加可写事件回调
    struct event *m_ev_write;
    struct timeval m_connect_timeout;
    bool m_bConnected;
};
#endif // COER_TCP_CLIENT_H