#ifndef SAFE_QUEUE_HPP
#define SAFE_QUEUE_HPP
#include <queue>
#include <mutex>
#include <condition_variable>
// #include "../../core/log/Log.hpp"

#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
    TypeName(const TypeName &) = delete;   \
    void operator=(const TypeName &) = delete

template <class Task>
class SafeQueue
{
public:
    SafeQueue(size_t max_size = INT32_MAX) : m_max_size(max_size)
    {
    }
    virtual ~SafeQueue()
    {
    }

public:
    void PushBack(std::shared_ptr<Task> &&task_ptr)
    {
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_pop_cv.wait(lock, [this]
                          { return this->m_queue.size() < this->m_max_size; });
            m_queue.emplace(std::move(task_ptr));
        }
        m_push_cv.notify_one();
    }

    std::shared_ptr<Task> PopFront()
    {
        std::shared_ptr<Task> task_ptr;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_push_cv.wait(lock, [this]
                           { return !this->m_queue.empty(); });
            task_ptr = std::move(m_queue.front());
            m_queue.pop();
        }
        m_pop_cv.notify_one();
        return task_ptr;
    }

    size_t Size()
    {
        std::lock_guard<std::mutex> guard(m_mutex);
        return m_queue.size();
    }

private:
    DISALLOW_COPY_AND_ASSIGN(SafeQueue);

private:
    size_t m_max_size;

    std::mutex m_mutex;

    std::condition_variable m_pop_cv;  // 通知pop的条件变量，当数据量已满时，等待
    std::condition_variable m_push_cv; // 通知push的条件变量，当无数据时，等待

    std::queue<std::shared_ptr<Task>> m_queue;
};
#endif // SAFE_QUEUE_HPP
