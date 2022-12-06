#ifndef EVENTLOOP_H
#define EVENTLOOP_H
#include <functional>
struct event_base;
class EventLoop
{
public:
    EventLoop();
    virtual ~EventLoop();

public:
    virtual bool Init();
    virtual void Loop();
    virtual void Stop();

    inline struct event_base *GetEventBase() { return m_base; }
    inline bool IsRunning() { return m_bIsRunning; }

private:
    bool m_bIsRunning;
    struct event_base *m_base;
};

#endif // EVENTLOOP_H