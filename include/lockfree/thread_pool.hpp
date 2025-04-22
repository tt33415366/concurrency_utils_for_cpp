#ifndef LOCKFREE_THREAD_POOL_HPP
#define LOCKFREE_THREAD_POOL_HPP

#include "queue.hpp"
#include <vector>
#include <thread>
#include <functional>
#include <atomic>
#include <future>
#include <memory>
#include <random>
#include <iostream>

namespace lockfree {

class ThreadPool {
public:
    using Task = std::function<void()>;

private:
    struct Worker {
        Queue<Task> local_queue;
        std::thread thread;
        std::atomic<bool> idle{false};
        
        Worker() = default;
        Worker(Worker&&) = delete;
        Worker& operator=(Worker&&) = delete;
    };

public:
    explicit ThreadPool(size_t num_threads = std::thread::hardware_concurrency());
    ~ThreadPool();

    template<typename F>
    auto submit(F&& f) -> std::future<decltype(f())> {
        if (!running_.load(std::memory_order_acquire)) {
            throw std::runtime_error("ThreadPool is shutdown");
        }

        using ReturnType = decltype(f());
        auto task = std::make_shared<std::packaged_task<ReturnType()>>(std::forward<F>(f));
        std::future<ReturnType> result = task->get_future();
        
        auto task_ptr = task.get();
        auto wrapped_task = [task = std::move(task), task_ptr]() {
            if (!task_ptr) return;
            try {
                (*task_ptr)();
            } catch (const std::exception& e) {
                std::cerr << "Exception in thread pool task: " << e.what() << "\n";
            } catch (...) {
                std::cerr << "Unknown exception in thread pool task\n";
            }
        };

        static std::atomic<size_t> next_worker{0};
        size_t worker_index = next_worker++ % workers_.size();
        
        workers_[worker_index]->local_queue.push(std::move(wrapped_task));
        active_tasks_.fetch_add(1, std::memory_order_release);
        
        return result;
    }

    void wait();
    void shutdown();

private:
    std::vector<std::unique_ptr<Worker>> workers_;
    Queue<Task> global_queue_;
    std::atomic<bool> running_{true};
    std::atomic<int> active_tasks_{0};
    std::atomic<int> completed_tasks_{0};
    std::atomic<size_t> idle_workers_{0};

    void worker_loop(size_t worker_id);
    bool steal_task(Task& task, size_t thief_id);
    size_t select_victim(size_t thief_id);
};

} // namespace lockfree

#include "thread_pool.ipp"

#endif // LOCKFREE_THREAD_POOL_HPP
