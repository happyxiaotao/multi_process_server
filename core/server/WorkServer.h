#ifndef SERVER_WORK_SERVER_H
#define SERVER_WORK_SERVER_H

#include <string>
#include <memory>
#include <thread>
#include <atomic>
#include "../pipe/PipePacket.h"
#include "../pipe/PipeChannel.h"
#include "../eventloop/EventLoop.h"
#include "../log/Log.hpp"

class WorkServer
{
    typedef std::function<void(PipePacket *)> FunctorPipePktCompleted;
    typedef std::function<void()> FunctorThreadLoopStart;

public:
    WorkServer(const std::string &thread_name, size_t thread_index)
        : m_thread_name(thread_name), m_thread_index(thread_index), m_thread_running(false)
    {
    }
    virtual ~WorkServer()
    {
    }

public:
    void SetPipeChannel(std::shared_ptr<PipeChannel> &&pipe_channel) { m_pipe_channel = std::move(pipe_channel); }

    inline bool IsRunning() { return m_thread_running.load() == true; }

    void Run()
    {
        auto f = [this]()
        {
            m_eventloop = std::make_shared<EventLoop>();
            m_eventloop->Init();

            this->m_pipe_channel->SetOnPipePktCompleted(std::bind(&WorkServer::OnPipePktCompleted, this, std::placeholders::_1));
            m_pipe_channel->Init(m_eventloop.get());

            this->OnThreadLoopStart();
            m_thread_running.store(true); // 放在OnThraedLoopStart之后，避免放在前面时，某些操作耗时，导致使用时还未初始化
            this->m_eventloop->Loop();
        };
        m_thread = std::make_shared<std::thread>(f);
    }
    void Stop()
    {
        m_thread_running.store(false);
    }
    void Join()
    {
        return m_thread->join();
    }
    inline EventLoop *GetEventLoop() { return m_eventloop.get(); }
    inline PipeChannel *GetPipeChannel() { return m_pipe_channel.get(); }
    inline size_t GetThreadIndex() { return m_thread_index; }

protected:
    virtual void OnPipePktCompleted(PipePacket *pkt) {}
    virtual void OnThreadLoopStart() {}

private:
    std::string m_thread_name;
    size_t m_thread_index;
    std::shared_ptr<std::thread> m_thread;
    std::shared_ptr<PipeChannel> m_pipe_channel;
    std::shared_ptr<EventLoop> m_eventloop;
    std::atomic_bool m_thread_running;
};

#endif // SERVER_WORK_SERVER_H