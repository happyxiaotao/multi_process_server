#include "TcpSession.h"
#include "../socket/Socket.h"
#include "../log/Log.hpp"
#include "../eventloop/EventLoop.h"

std::atomic_uint64_t TcpSession::s_total_session_count{0};
session_id_t TcpSession::GenerateSessionId()
{
    constexpr int OneMillion = 1000000;
    int cur_session_count = ++s_total_session_count; // 使用原子操作的累加，避免多线程下的异常
    unsigned long long t = time(nullptr);            // 使用unsigned long long是保证32位环境下，t也占用64字节，这样后续处理时，可以保证数据容量够大
    return t * OneMillion + cur_session_count % OneMillion;
}

void TcpSession::tcp_session_read_cb(evutil_socket_t socket, short events, void *args)
{
    TcpSession *session = static_cast<TcpSession *>(args);
    session->HandleReadEvent(socket, events);
}

TcpSession::TcpSession() : m_remote_port(INVALID_PORT), m_session_id(GenerateSessionId()), m_eventloop(nullptr),
                           m_fd(INVALID_SOCKET), m_read_event(nullptr)
{
    Trace("TcpSession::TcpSession");
}
TcpSession::~TcpSession()
{
    Trace("TcpSession::~TcpSession");
    Clear();
}

bool TcpSession::InitReadEvent(int fd)
{
    return Init(GetEventLoop(), fd, m_remote_ip, m_remote_port);
}
bool TcpSession::Init(EventLoop *eventloop, int fd, const std::string &remote_ip, u_short remote_port)
{
    if (eventloop == nullptr || fd == INVALID_SOCKET)
    {
        Trace("TcpSession::Init, invalid args,fd={}", fd);
        return false;
    }
    if (m_eventloop != nullptr && m_eventloop != eventloop)
    {
        m_eventloop = eventloop;
        Warn("TcpSession::Init, eventloop != m_eventloop");
    }
    SetEventLoop(eventloop);
    SetRemoteIp(remote_ip);
    SetRemotePort(remote_port);
    m_fd = fd;

    evutil_make_socket_closeonexec(m_fd);
    evutil_make_socket_nonblocking(m_fd);

    struct event *event = event_new(m_eventloop->GetEventBase(), m_fd, EV_READ | EV_PERSIST, &TcpSession::tcp_session_read_cb, this);
    if (event == nullptr)
    {
        Clear();
        return false;
    }
    m_read_event = event;
    event_add(m_read_event, nullptr);

    return true;
}

void TcpSession::HandleReadEvent(evutil_socket_t socket, short events)
{
    Buffer buffer;
    int nRecv = buffer.GetDataFromFd(socket);
    if (nRecv <= 0)
    {
        // Error("TcpSession::HandlerEvent, socket={}, events=0x{:x},nRecv={},errno={}", socket, events, nRecv, errno);
    }

    auto self = shared_from_this();
    if (nRecv > 0)
    {
        if (m_read_callback)
        {
            m_read_callback(self, buffer);
        }
        else
        {
            OnMessage(self, buffer);
        }
    }
    else if (nRecv == 0)
    {
        TcpErrorType type = TCP_ERROR_DISCONNECT;
        if (m_error_callback)
        {
            m_error_callback(self, type);
        }
        else
        {
            OnError(self, type);
        }
    }
    else
    {
        TcpErrorType type = TCP_ERROR_NET_ERROR;
        if (m_read_callback)
        {
            m_error_callback(self, type);
        }
        else
        {
            OnError(self, type);
        }
    }
}
ssize_t TcpSession::SendBuffer(const struct iovec *iov, int iovcnt, int &nerrno)
{
    int nwritten = 0;
    nerrno = 0;
    do
    {
        nwritten = writev(m_fd, iov, iovcnt);
        nerrno = errno;
    } while (nwritten < 0 && nerrno == EINTR);
    if (nwritten < 0)
    {
        size_t len = 0;
        for (int i = 0; i < iovcnt; i++)
        {
            len += iov[i].iov_len;
        }
        if (!(nerrno == EAGAIN || nerrno == EWOULDBLOCK))
        {
            Error("TcpSession::SendBuffer, use function writev, fd:{},length:{},nwritten:{},errno:{},error:{}",
                  m_fd, len, nwritten, nerrno, strerror(nerrno));
        }
    }
    return nwritten;
}
ssize_t TcpSession::SendBuffer(const BufferPtr &buffer)
{
    return SendBuffer(buffer->GetBuffer(), buffer->ReadableBytes());
}
ssize_t TcpSession::SendBuffer(const char *buffer, size_t len)
{
    int nwritten = 0;
    int nerrno = 0;
    do
    {
        nwritten = send(m_fd, buffer, len, 0);
        nerrno = errno;
    } while (nwritten < 0 && nerrno == EINTR);
    if (nwritten < 0)
    {
        Error("TcpSession::SendBuffer,use function send, fd:{},length:{},nwritten:{},errno:{},error:{}",
              m_fd, len, nwritten, nerrno, strerror(nerrno));
    }
    return nwritten;
}

void TcpSession::Clear()
{
    m_eventloop = nullptr;
    if (m_read_event != nullptr)
    {
        if (event_pending(m_read_event, EV_READ | EV_TIMEOUT | EV_WRITE | EV_SIGNAL, nullptr))
        {
            event_del(m_read_event);
        }
        event_free(m_read_event);
        m_read_event = nullptr;
    }
    if (m_fd != INVALID_SOCKET)
    {
        evutil_closesocket(m_fd);
        m_fd = INVALID_SOCKET;
    }
    m_read_callback = nullptr;
    m_error_callback = nullptr;
}
