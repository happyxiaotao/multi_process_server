#ifndef SERVER_MASTER_SERVER_H
#define SERVER_MASTER_SERVER_H

#include <vector>
#include <string>
#include <atomic>
#include <thread>
#include <memory>
#include <functional>
#include "../pipe/PipePktType.h"
#include "../pipe/PipeFds.h"
#include "../pipe/PipeChannel.h"
#include "../pipe/PipePacket.h"
#include "../eventloop/EventLoop.h"
#include "WorkServer.h"
#include "../log/Log.hpp"
// T是WorkServer类或其派生类
template <typename T = WorkServer>
class MasterServer
{
    typedef std::function<void()> FunctorThreadLoopStart;
    typedef std::function<void(PipePacket *pkt)> FunctorPipePktCompleted;

public:
    MasterServer(const std::string &thread_name)
        : m_thread_name(thread_name) {}
    virtual ~MasterServer()
    {
        for (size_t i = 0; i < m_work_servers.size(); i++)
        {
            NotifyStopThread(i);
            while (!m_work_servers[i]->IsRunning())
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(5)); // wait for 5ms
                Trace("MasterServer::~MasterServer,wait for stop thread");
            }
        }
        for (size_t i = 0; i < m_work_servers.size(); i++)
        {
            auto &work_server = m_work_servers[i];
            work_server->Join();
        }
        m_work_servers.clear();
    }

public:
    bool Init(size_t thread_num)
    {
        m_eventloop = std::make_shared<EventLoop>();
        if (!m_eventloop->Init())
        {
            Error("MasterServer::Init,eventloop init failed!thread_num={}", thread_num);
            return false;
        }

        m_pipe_channels.resize(thread_num);
        m_work_servers.resize(thread_num);
        m_pipe_fds.resize(thread_num);

        for (size_t i = 0; i < thread_num; i++)
        {
            std::string thread_name = m_thread_name + "_work_thread_" + std::to_string(i);
            CreateWorkServer(thread_name, i);
        }
        return true;
    }

    void Run(int wait_interval_ms = 0)
    {
        Trace("MasterServer::Run, thread_name:{}", m_thread_name);
        std::this_thread::sleep_for(std::chrono::milliseconds(wait_interval_ms));
        for (size_t i = 0; i < m_work_servers.size(); i++)
        {
            m_work_servers[i]->Run();
            while (!m_work_servers[i]->IsRunning())
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(5)); // wait for 5ms
            }
        }

        if (m_functor_thread_loop_start)
        {
            m_functor_thread_loop_start();
        }
        m_eventloop->Loop();
    }

    void SetOnThreadLoopStart(FunctorThreadLoopStart functor) { m_functor_thread_loop_start = functor; }
    void SetOnPipePktCompleted(FunctorPipePktCompleted functor) { m_functor_pipe_pkt_completed = functor; }

    inline EventLoop *GetEventLoop() { return m_eventloop.get(); }
    inline PipeChannel *GetPipeChannel(size_t thread_index) { return m_pipe_channels[thread_index].get(); }
    inline size_t GetThreadNum() { return m_work_servers.size(); }

private:
    // bool CreateWorkServer(const std::string &thread_name, size_t thread_index);
    bool CreateWorkServer(const std::string &thread_name, size_t thread_index)
    {
        Trace("CreateWorkServer,thread_name={}", thread_name);
        auto pipe_fds = std::make_shared<PipeFds>();
        pipe_fds->Init();

        auto master_pipe_channel = std::make_shared<PipeChannel>(pipe_fds->GetPipeMasterReadFd(), pipe_fds->GetPipeMasterWriteFd());
        auto work_pipe_channel = std::make_shared<PipeChannel>(pipe_fds->GetPipeThreadReadFd(), pipe_fds->GetPipeThreadWriteFd());

        auto thread = std::make_shared<T>(thread_name, thread_index);
        thread->SetPipeChannel(std::move(work_pipe_channel));

        master_pipe_channel->SetOnPipePktCompleted(m_functor_pipe_pkt_completed);
        master_pipe_channel->Init(m_eventloop.get());

        m_pipe_channels[thread_index] = std::move(master_pipe_channel);
        m_work_servers[thread_index] = std::move(thread);
        m_pipe_fds[thread_index] = std::move(pipe_fds);

        return true;
    }

    bool NotifyStopThread(size_t thread_index)
    {
        auto type = PipePktType::PIPE_PKT_STOP_THREAD;
        const std::string msg("stop thread");
        return (size_t)SendMsgToWorkThread(thread_index, type, msg) == msg.size();
    }
    int SendMsgToWorkThread(size_t thread_index, PipePktType type, const std::string &msg)
    {
        return m_pipe_channels[thread_index]->WriteBuffer(type, msg.c_str(), msg.size());
    }

private:
    std::string m_thread_name;
    std::vector<std::shared_ptr<T>> m_work_servers;
    std::vector<std::shared_ptr<PipeChannel>> m_pipe_channels;
    std::shared_ptr<EventLoop> m_eventloop;
    std::vector<std::shared_ptr<PipeFds>> m_pipe_fds; // 负责创建fds，并释放fds

    FunctorThreadLoopStart m_functor_thread_loop_start;
    FunctorPipePktCompleted m_functor_pipe_pkt_completed;
};

#endif // SERVER_MASTER_SERVER_H