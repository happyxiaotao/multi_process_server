#ifndef WORK_THREAD_HPP
#define WORK_THREAD_HPP

#include <thread>
#include <functional>
#include <atomic>
#include "SafeQueue.hpp"
#include "../../core/log/Log.hpp"

template <class Task>
class WorkThread
{
public:
    WorkThread() : m_bThreadRunning(false) {}
    virtual ~WorkThread()
    {
        Stop();
        Join();
    }

public:
    void Start(const std::string &strThreadName, bool bWaitForRunning = true)
    {
        if (!m_thread)
        {
            m_thread = std::make_shared<std::thread>(std::bind(&WorkThread::Run, this, strThreadName));

            // 等待线程开启
            if (bWaitForRunning)
            {
                while (!IsThreadRunning())
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));
                }
            }
        }
    }

    // 必须实现关闭线程功能，通过调用PostTask来实现
    void Stop()
    {
        PostTask(std::move(CreateTerminateTask()));
    }

    // 子线程运行的地方
    void Run(std::string strThreadName)
    {
        // 设置线程名称
        pthread_setname_np(pthread_self(), strThreadName.c_str());

        m_bThreadRunning.store(true);

        // 子线程一直运行，进行消费。如果SafeQueue中数据为空，则会等待
        for (;;)
        {
            std::shared_ptr<Task> task_ptr = std::move(m_queue.PopFront());
            if (task_ptr && IsTerminateTask(task_ptr))
            {
                break;
            }
            HandlerTask(task_ptr);
        }
    }

    void Join()
    {
        if (m_thread && m_thread->joinable())
        {
            m_thread->join();
        }
        m_bThreadRunning.store(false);
    }

    bool IsThreadRunning() { return m_bThreadRunning.load(); }
    size_t QueueSize() { return m_queue.Size(); }

protected:
    void PostTask(Task &&task) { m_queue.PushBack(std::make_shared<Task>(std::move(task))); }
    void PostTask(std::shared_ptr<Task> &&task_ptr)
    {
        m_queue.PushBack(std::move(task_ptr));
    }

protected:
    // 派生类实现：处理任务
    virtual void HandlerTask(const std::shared_ptr<Task> &task) = 0;
    // 派生类实现：创建表示退出线程的task
    virtual std::shared_ptr<Task> CreateTerminateTask() = 0;
    // 派生类实现：判断是否应该退出线程
    virtual bool IsTerminateTask(const std::shared_ptr<Task> &task) = 0;

private:
    std::shared_ptr<std::thread> m_thread;
    SafeQueue<Task> m_queue;
    std::atomic_bool m_bThreadRunning;
};

#endif // WORK_THREAD_HPP