#ifndef PIPE_CHANNEL_H
#define PIPE_CHANNEL_H

#include <string>
#include <functional>
#include "PipePacket.h"
#include "../buffer/Buffer.h"

#define INVALID_SOCKET_FD (-1)

class EventLoop;
class PipeChannel
{
    typedef std::function<void(PipePacket *pkt)> Handler;

public:
    PipeChannel(int read_fd, int write_fd);
    ~PipeChannel();

public:
    void SetOnPipePktCompleted(Handler handler) { m_handler = handler; }
    int WriteBuffer(uint32_t type, const char *buffer, size_t len);
    bool Init(EventLoop *eventloop);

private:
    static void __pipe_event_cb(int, short, void *);

    void Clear();
    void OnRead();
    void ParseBuffer(const char *buffer, size_t len);

    bool ParseHeader();
    bool ParseBody();

private:
    enum enumReadStatus
    {
        kReadHeader, // 读取包头
        kReadData,   // 读取数据
    };
    enum
    {
        READ_BUFF_SIZE = 1024
    };

    EventLoop *m_eventloop;
    int m_read_fd;
    int m_write_fd;

    enumReadStatus m_read_status;
    struct event *m_read_event;

    ssize_t m_howmuch;
    ssize_t m_left_to_read;
    PipePacket *m_tmp_pkt;
    Buffer m_read_buffer;
    Handler m_handler;
};

#endif // PIPE_CHANNEL_H