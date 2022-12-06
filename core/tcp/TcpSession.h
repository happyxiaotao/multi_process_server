#ifndef TCP_CONN_H
#define TCP_CONN_H

#include <string>
#include <atomic>
#include <memory>
#include <functional>
#include <event2/event.h>
#include <sys/uio.h>
#include "../buffer/Buffer.h"
#include "TcpError.h"

class EventLoop;
class TcpSession;
typedef uint64_t session_id_t;
typedef std::shared_ptr<TcpSession> TcpSessionPtr;

class TcpSession : public std::enable_shared_from_this<TcpSession>
{
public:
    typedef std::function<void(const TcpSessionPtr &session, TcpErrorType error_type)> ErrorCallBack;
    typedef std::function<void(const TcpSessionPtr &session, const Buffer &buffer)> ReadCallBack;

public:
    TcpSession();
    virtual ~TcpSession();

public:
    bool InitReadEvent(int fd);
    virtual bool Init(EventLoop *eventloop, int fd, const std::string &remote_ip, u_short remote_port);
    inline void SetReadCallBack(const ReadCallBack &cb) { m_read_callback = cb; }
    inline void SetEventCallBack(const ErrorCallBack &cb) { m_error_callback = cb; }
    inline evutil_socket_t GetSocketFd() { return m_fd; }
    inline session_id_t GetSessionId() const { return m_session_id; }
    inline EventLoop *GetCurThreadEventLoop() { return m_eventloop; }
    inline void SetRemoteIp(const std::string &remote_ip) { m_remote_ip = remote_ip; }
    inline void SetRemotePort(u_short remote_port) { m_remote_port = remote_port; }
    inline const std::string &GetRemoteIp() const { return m_remote_ip; }
    inline u_short GetRemotePort() { return m_remote_port; }
    inline void SetEventLoop(EventLoop *eventloop) { m_eventloop = eventloop; }
    inline EventLoop *GetEventLoop() { return m_eventloop; }

    ssize_t SendBuffer(const struct iovec *iov, int iovcnt, int &nerrno);
    ssize_t SendBuffer(const BufferPtr &buffer);
    ssize_t SendBuffer(const char *buffer, size_t len);

    virtual void Clear();

protected:
    virtual void OnMessage(const TcpSessionPtr &tcp, const Buffer &buffer) {}
    virtual void OnError(const TcpSessionPtr &tcp, TcpErrorType error_type) {}

private:
    void HandleReadEvent(evutil_socket_t socket, short events);

private:
    static session_id_t GenerateSessionId();
    static std::atomic_uint64_t s_total_session_count;
    static void tcp_session_read_cb(evutil_socket_t socket, short events, void *args);

private:
    std::string m_remote_ip;
    u_short m_remote_port;
    session_id_t m_session_id;
    EventLoop *m_eventloop;
    evutil_socket_t m_fd;
    struct event *m_read_event;

    ErrorCallBack m_error_callback;
    ReadCallBack m_read_callback;
};

#endif // TCP_CONN_H