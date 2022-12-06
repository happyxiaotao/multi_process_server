#include "./EventWatcher.h"

EventWatcher::EventWatcher(struct event_base *base, const Functor &handler) : m_base(base), m_event(nullptr), m_handler(handler)
{
    evutil_timerclear(&m_timeout);
    m_event = new event;
    memset(m_event, 0, sizeof(struct event));
}
EventWatcher::EventWatcher(struct event_base *base, Functor &&handler) : m_base(base), m_event(nullptr), m_handler(std::move(handler))
{
    evutil_timerclear(&m_timeout);
    m_event = new event;
    memset(m_event, 0, sizeof(struct event));
}

EventWatcher::~EventWatcher()
{
    FreeEvent();
    Close();
}
bool EventWatcher::Init()
{
    if (!DoInit())
    { // DoInit里面会设置event对应的监听事件
        Close();
        return false;
    }
    if (event_get_base(m_event) != m_base)
    {
        event_base_set(m_base, m_event);
    }
    if (evutil_timerisset(&m_timeout))
    {
        event_add(m_event, &m_timeout);
    }
    else
    {
        event_add(m_event, nullptr);
    }
    return true;
}

void EventWatcher::Cancel()
{
    if (m_event == nullptr)
    {
        return;
    }
    FreeEvent();
    if (m_cancel)
    {
        m_cancel();
    }
}
void EventWatcher::FreeEvent()
{
    if (m_event != nullptr)
    {
        if (event_pending(m_event, EV_READ | EV_WRITE | EV_SIGNAL | EV_TIMEOUT, nullptr))
        {
            event_del(m_event);
            delete m_event;
            m_event = nullptr;
        }
    }
}
