#ifndef SWORD_TIMER_EVENT_WATCHER_H
#define SWORD_TIMER_EVENT_WATCHER_H

#include "EventWatcher.h"

class EventLoop;
class TimerEventWatcher : public EventWatcher
{
public:
    TimerEventWatcher(EventLoop *eventloop, const Functor &functor, const struct timeval &timeout);
    TimerEventWatcher(EventLoop *eventloop, Functor &&functor, const struct timeval &timeout);
    ~TimerEventWatcher();

public:
    bool StartTimer();

protected:
    virtual bool DoInit() override;

private:
    static void OnTimer(evutil_socket_t socket, short events, void *args);

private:
    bool m_bWaitTimer; //是否正在等待超时事件，避免多次调用
};

#endif // SWORD_TIMER_EVENT_WATCHER_H