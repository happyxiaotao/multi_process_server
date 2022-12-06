#include "TimerEventWatcher.h"
#include "../eventloop/EventLoop.h"
#include "../log/Log.hpp"

TimerEventWatcher::TimerEventWatcher(EventLoop *eventloop, const Functor &functor, const struct timeval &timeout)
    : EventWatcher(eventloop->GetEventBase(), functor), m_bWaitTimer(false)
{
    memcpy(&m_timeout, &timeout, sizeof(timeout));
}
TimerEventWatcher::TimerEventWatcher(EventLoop *eventloop, Functor &&functor, const struct timeval &timeout)
    : EventWatcher(eventloop->GetEventBase(), std::move(functor)), m_bWaitTimer(false)
{
    memcpy(&m_timeout, &timeout, sizeof(timeout));
}
TimerEventWatcher::~TimerEventWatcher()
{
    Close();
}

bool TimerEventWatcher::DoInit()
{
    // 这里避免了重新赋值m_event
    if (event_get_base(m_event) != nullptr)
    {
        return true;
    }
    if (event_assign(m_event, m_base, -1, EV_TIMEOUT, TimerEventWatcher::OnTimer, this) < 0)
    {
        Close();
        return false;
    }
    return true;
}
bool TimerEventWatcher::StartTimer()
{
    if (m_bWaitTimer)
    {
        return false;
    }
    m_bWaitTimer = true;
    return Init();
}
void TimerEventWatcher::OnTimer(evutil_socket_t socket, short events, void *args)
{
    TimerEventWatcher *watcher = static_cast<TimerEventWatcher *>(args);
    watcher->m_bWaitTimer = false;
    watcher->m_handler();
}
