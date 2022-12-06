#ifndef SWORD_EVENT_WATCHER_H
#define SWORD_EVENT_WATCHER_H

#include <functional>
#include <string.h>
#include <event2/event.h>
#include <event2/event_struct.h>

class EventWatcher
{
public:
    typedef std::function<void()> Functor;

    virtual ~EventWatcher();

    bool Init();

    void Cancel();

    inline void SetCancelCallback(const Functor &cb) { m_cancel = cb; }

    void ClearHandler() { m_handler = Functor(); }

    void FreeEvent();

protected:
    EventWatcher(struct event_base *base, const Functor &handler);
    EventWatcher(struct event_base *base, Functor &&handler);

    void Close() { DoClose(); }
    virtual bool DoInit() = 0;
    virtual void DoClose() {}

protected:
    struct event_base *m_base;
    struct event *m_event;
    struct timeval m_timeout;
    Functor m_handler;
    Functor m_cancel;
};

#endif // SWORD_EVENT_WATCHER_H