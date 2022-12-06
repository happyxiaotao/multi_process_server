#include <event2/event.h>
#include "EventLoop.h"
EventLoop::EventLoop() : m_bIsRunning(false), m_base(nullptr)
{
}
EventLoop::~EventLoop()
{
    if (m_base)
    {
        event_base_free(m_base);
        m_base = nullptr;
    }
    m_bIsRunning = false;
}

bool EventLoop::Init()
{
#if LIBEVENT_VERSION_NUMBER >= 0x02001500
    struct event_config *cfg = event_config_new();
    if (cfg)
    {
        // Does not cache time to get a preciser timer
        event_config_set_flag(cfg, EVENT_BASE_FLAG_NO_CACHE_TIME);
        m_base = event_base_new_with_config(cfg);
        event_config_free(cfg);
    }
#else
    m_base = event_base_new();
#endif
    return m_base != nullptr;
}
void EventLoop::Loop()
{
    if (m_base)
    {
        m_bIsRunning = true;
        event_base_dispatch(m_base);
    }
}
void EventLoop::Stop()
{
    if (m_base)
    {
        event_base_loopexit(m_base, nullptr);
    }
    m_bIsRunning = false;
}