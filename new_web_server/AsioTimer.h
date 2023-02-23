#ifndef ASIO_TIMER_H
#define ASIO_TIMER_H
// 基于asio的定时器

#include <asio.hpp>
#include <asio/deadline_timer.hpp>

class AsioTimer
{
    typedef std::function<void()> FunctionTimer;

public:
    AsioTimer(asio::io_context &io_context) : m_io_context(io_context), m_timer(io_context),
                                              m_is_waitting(false)
    {
    }
    ~AsioTimer()
    {
        StopTimer();
    }

    void SetFuncTimer(FunctionTimer func) { m_func = func; }

    void StartTimer(int nSeconds)
    {
        if (m_is_waitting)
        {
            return;
        }
        m_is_waitting = true;
        m_timer.expires_after(std::chrono::seconds(nSeconds));
        m_timer.async_wait([this](const std::error_code &)
                           { if (this->m_func){this->m_func();} 
                           this->m_is_waitting=false; });
    }
    void StopTimer()
    {
        m_timer.cancel();
        m_is_waitting = false;
    }

public:
    asio::io_context &m_io_context;
    FunctionTimer m_func;
    asio::steady_timer m_timer;
    bool m_is_waitting;
};

#endif // ASIO_TIMER_H