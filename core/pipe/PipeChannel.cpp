#include <sys/uio.h>
#include <event2/util.h>
#include <event2/event.h>
#include <string.h>
#include <unistd.h>
#include "PipeChannel.h"
#include "../eventloop/EventLoop.h"
#include "../../core/log/Log.hpp"

PipeChannel::PipeChannel(int read_fd, int write_fd)
    : m_eventloop(nullptr), m_read_fd(read_fd), m_write_fd(write_fd),
      m_read_status(kReadHeader), m_read_event(nullptr),
      m_howmuch(sizeof(PipePacket)), m_left_to_read(0),
      m_tmp_pkt(nullptr)
{
    Trace("read_fd={},write_fd={}", read_fd, write_fd);
}

PipeChannel::~PipeChannel()
{
}

bool PipeChannel::Init(EventLoop *eventloop)
{
    if (eventloop == nullptr)
    {
        Error("PipeChannel::Init failed,eventloop=nullptr");
        return false;
    }

    m_eventloop = eventloop;

    evutil_make_socket_nonblocking(m_read_fd);
    evutil_make_socket_closeonexec(m_read_fd);
    evutil_make_socket_nonblocking(m_write_fd);
    evutil_make_socket_closeonexec(m_write_fd);

    // 监听可读
    // 不用专门设置阻塞和非阻塞。外界调用pipe2创建管道fds时候，会设置非阻塞属性
    struct event *read_ev = event_new(m_eventloop->GetEventBase(), m_read_fd, EV_READ | EV_PERSIST, PipeChannel::__pipe_event_cb, this);
    if (read_ev == nullptr)
    {
        Clear();
        Error("CPipeSession::Init event_new failed!");
        return false;
    }
    m_read_event = read_ev;
    event_add(m_read_event, nullptr);
    Trace("PipeChannel::Init,watch read_fd:{},write_fd:{}", m_read_fd, m_write_fd);

    return true;
}

void PipeChannel::Clear()
{
    if (m_read_fd > 0)
    {
        m_read_fd = INVALID_SOCKET_FD;
    }
    if (m_write_fd > 0)
    {
        m_write_fd = INVALID_SOCKET_FD;
    }
    if (m_read_event)
    {
        event_del(m_read_event);
        event_free(m_read_event);
        m_read_event = nullptr;
    }
    if (m_eventloop)
    {
        m_eventloop = nullptr;
    }
    m_read_status = kReadHeader;
    m_howmuch = sizeof(PipePacket);
    m_left_to_read = 0;
    m_tmp_pkt = nullptr;
    m_read_buffer.Clear();
}

int PipeChannel::WriteBuffer(uint32_t type, const char *buffer, size_t len)
{
    PipePacket packet;
    packet.m_uDataLength = len;
    packet.m_uPktType = type;
    struct iovec vec[2];
    vec[0].iov_base = &packet;
    vec[0].iov_len = packet.m_uHeadLength;
    vec[1].iov_base = (void *)buffer;
    vec[1].iov_len = len;

    return writev(m_write_fd, vec, 2);
}
void PipeChannel::__pipe_event_cb(int fd, short events, void *arg)
{
    PipeChannel *channel = static_cast<PipeChannel *>(arg);
    if ((events & EV_READ) == EV_READ)
    {
        channel->OnRead();
    }
}

void PipeChannel::OnRead()
{
    int ntoread = std::min<ssize_t>(m_howmuch, READ_BUFF_SIZE); // 找到最少需要读取的字节数
    if (m_left_to_read != 0)
    {
        ntoread = std::min<ssize_t>(m_left_to_read, ntoread); // 剩余未读的，需要读取到
    }
    ssize_t nread = m_read_buffer.GetDataFromFd(m_read_fd, ntoread);
    if (nread <= 0)
    {
        Error("PipeChannel::OnRead,m_read_fd={}", m_read_fd);
        return;
    }
    if (m_left_to_read != 0)
    {
        m_left_to_read -= nread;
    }
    else
    {
        m_left_to_read = m_howmuch - nread;
    }
    if (m_left_to_read != 0)
    {
        return; //等下次读取之后进行处理
    }

    if (m_read_status == kReadHeader)
    {
        m_tmp_pkt = (PipePacket *)m_read_buffer.GetBuffer();
        m_howmuch = m_tmp_pkt->m_uDataLength;
        m_read_status = kReadData;
    }
    else if (m_read_status == kReadData)
    {
        m_tmp_pkt = (PipePacket *)m_read_buffer.GetBuffer();
        if (m_handler)
        {
            m_handler(m_tmp_pkt);
        }
        m_howmuch = sizeof(PipePacket);
        m_read_status = kReadHeader;
        m_read_buffer.Clear();
    }
}
