#ifndef LOCKFREE_THREAD_POOL_HPP
#define LOCKFREE_THREAD_POOL_HPP

#include "queue.hpp"
#include <vector>
#include <thread>
#include <functional>
#include <atomic>
#include <future>
#include <memory>

namespace lockfree {

class ThreadPool {
public:
    explicit ThreadPool(size_t num_threads = std::thread::hardware_concurrency());
    ~ThreadPool();

    // Submit a task to the pool
    template<typename F>
    auto submit(F&& f) -> std::future<decltype(f())> {
        if (!running_.load(std::memory_order_acquire)) {
            throw std::runtime_error("ThreadPool is shutdown");
        }

        using ReturnType = decltype(f());
        auto task = std::make_shared<std::packaged_task<ReturnType()>>(std::forward<F>(f));
        std::future<ReturnType> result = task->get_future();
        
        // Limit concurrent tasks to prevent overload
        while (pending_tasks_.load(std::memory_order_acquire) > 1000) {
            std::this_thread::yield();
        }

        auto wrapped_task = [task, this]{
            (*task)();
            active_tasks_.fetch_sub(1, std::memory_order_release);
            pending_tasks_.fetch_sub(1, std::memory_order_release);
        };
        
        pending_tasks_.fetch_add(1, std::memory_order_release);
        active_tasks_.fetch_add(1, std::memory_order_release);
        task_queue_.push(std::move(wrapped_task));
        
        return result;
    }

    // Wait for all tasks to complete
    void wait();

    // Stop accepting new tasks and wait for completion
    void shutdown();

private:
    using Task = std::function<void()>;
    
    Queue<Task> task_queue_;
    std::vector<std::thread> workers_;
    std::atomic<bool> running_{true};
    std::atomic<int> active_tasks_{0};
    std::atomic<int> pending_tasks_{0};

    void worker_loop();
};

} // namespace lockfree

#include "thread_pool.ipp"

#endif // LOCKFREE_THREAD_POOL_HPP
