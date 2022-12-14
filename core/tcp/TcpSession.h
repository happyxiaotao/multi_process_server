#ifndef TCP_CONN_H
#define TCP_CONN_H

#include <string>
#include <atomic>
#include <memory>
#include <functional>
#include <event2/event.h>
#include <list>
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

    bool IsCanWrite() { return m_bFdCanWrite; }
    ssize_t SendBuffer(const struct iovec *iov, int iovcnt, int &nerrno);
    ssize_t SendBuffer(const BufferPtr &buffer, int &nerrno);
    ssize_t SendBuffer(const char *buffer, size_t len, int &nerrno);

    virtual void Clear();

protected:
    virtual void OnMessage(const TcpSessionPtr &tcp, const Buffer &buffer) {}
    virtual void OnError(const TcpSessionPtr &tcp, TcpErrorType error_type) {}

protected:
    // 下面是处理，发送失败后，添加到缓存区的数据，需要调用者调用来发送或添加
    bool EmptyPendingBuffer() { return m_pending_buffer_list.empty(); }
    size_t SizePendingBuffer() { return m_pending_buffer_list.size(); }
    void PushBackPendingBuffer(const char *buffer, size_t len) { m_pending_buffer_list.push_back(std::move(std::string(buffer, len))); }
    void PushBackPendingBuffer(std::string &&buffer) { m_pending_buffer_list.push_back(std::move(buffer)); }
    void PushFrontPendingBuffer(const char *buffer, size_t len) { m_pending_buffer_list.push_front(std::move(std::string(buffer, len))); }
    void PushFrontPendingBuffer(std::string &&buffer) { m_pending_buffer_list.push_front(std::move(buffer)); }
    const std::string &FrontPendingBuffer() const { return m_pending_buffer_list.front(); }
    void PopFrontPendingBuffer() { m_pending_buffer_list.pop_front(); }
    bool SendPendingBuffer();

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

    bool m_bFdCanWrite;                           // socket是否可写，如果对端关闭了socket，则不可写。每次写入前，需要判断下，否则会有coredump
    std::list<std::string> m_pending_buffer_list; // 未发送成功的数据缓存，需要由调用者存入或取出
};

#endif // TCP_CONN_H