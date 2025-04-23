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
#include <mutex>

namespace lockfree {

class ThreadPool {
public:
    using Task = std::function<void()>;

private:
    struct alignas(64) Worker {  // Cache line alignment
        Queue<Task> local_queue;
        std::atomic<bool> idle;
        std::thread thread;
        std::atomic<bool> valid{true};
        
        Worker() : idle(false) {
            std::cerr << "Worker constructor called\n";
        }
        ~Worker() {
            std::cerr << "Worker destructor started\n";
            valid.store(false, std::memory_order_release);
            
            // Ensure thread is stopped
            if (thread.joinable()) {
                std::cerr << "Joining worker thread...\n";
                try {
                    thread.join();
                    std::cerr << "Worker thread joined successfully\n";
                } catch (...) {
                    std::cerr << "Error joining worker thread\n";
                }
            }
            
            // Safe queue clearing
            std::cerr << "Clearing worker queue...\n";
            try {
                Task task;
                while (true) {
                    std::cerr << "Attempting to pop task...\n";
                    if (!local_queue.pop(task)) {
                        std::cerr << "Queue empty, stopping\n";
                        break;
                    }
                    std::cerr << "Popped task\n";
                    if (task) {
                        try {
                            std::cerr << "Executing task\n";
                            task();
                        } catch (...) {
                            std::cerr << "Task execution error\n";
                        }
                    }
                }
            } catch (const std::exception& e) {
                std::cerr << "Queue clearing error: " << e.what() << "\n";
            } catch (...) {
                std::cerr << "Unknown queue clearing error\n";
            }
            
            std::cerr << "Worker destructor completed\n";
        }
        
        bool is_valid() const {
            return valid.load(std::memory_order_acquire);
        }
        
        void stop() {
            valid.store(false, std::memory_order_release);
            // Wake up the worker if it's waiting
            local_queue.push([]{}); // Push empty task
            if (thread.joinable()) {
                thread.join();
            }
        }
    };

    std::vector<std::shared_ptr<Worker>> workers_;
    Queue<Task> global_queue_;
    std::atomic<bool> running_{true};
    std::atomic<int> active_tasks_{0};
    // Performance counters
    std::atomic<size_t> tasks_executed_{0};
    std::atomic<size_t> tasks_stolen_{0};
    std::atomic<size_t> steal_attempts_{0};
    struct PromiseHolder {
        virtual ~PromiseHolder() = default;
        virtual void set_exception(std::exception_ptr e) = 0;
    };

    template<typename T>
    struct TypedPromiseHolder : PromiseHolder {
        std::shared_ptr<std::promise<T>> promise;
        
        explicit TypedPromiseHolder(std::shared_ptr<std::promise<T>> p) : promise(p) {}
        
        void set_exception(std::exception_ptr e) override {
            try {
                promise->set_exception(e);
            } catch (...) {
                // Ignore set_exception errors
            }
        }
    };

    std::mutex pending_promises_mutex_;
    std::vector<std::unique_ptr<PromiseHolder>> pending_promises_;

public:
    explicit ThreadPool(size_t num_threads = std::thread::hardware_concurrency());
    
    ~ThreadPool() {
        std::cerr << "ThreadPool destructor called\n";
        try {
            if (running_.exchange(false, std::memory_order_release)) {
                std::cerr << "Shutting down workers...\n";
                
                // Phase 1: Cancel all pending promises
                {
                    auto promises = std::move(pending_promises_);
                    pending_promises_.clear();
                    
                    for (auto it = promises.rbegin(); it != promises.rend(); ++it) {
                        if (*it) {
                            try {
                                (*it)->set_exception(std::make_exception_ptr(
                                    std::runtime_error("ThreadPool shutdown")));
                            } catch (...) {
                                // Ignore any errors
                            }
                        }
                    }
                }

                // Phase 2: Stop all workers
                for (auto& worker : workers_) {
                    if (worker) {
                        worker->stop();
                    }
                }
                
                // Phase 3: Clear all queues
                global_queue_.clear();
                for (auto& worker : workers_) {
                    if (worker) {
                        worker->local_queue.clear();
                    }
                }
                
                // Phase 4: Safety checks
                active_tasks_.store(0, std::memory_order_release);
                size_t remaining_nodes = Queue<Task>::get_active_nodes();
                if (remaining_nodes > 0) {
                    std::cerr << "Force releasing " << remaining_nodes << " remaining nodes\n";
                    Queue<Task>::force_release_nodes();
                }
            }
            
            std::cerr << "ThreadPool destructor completed\n";
        } catch (...) {
            std::cerr << "Exception in ThreadPool destructor\n";
        }
    }

    template<typename F>
    auto submit_impl(F&& f, std::true_type) -> std::future<void> {
        auto promise = std::make_shared<std::promise<void>>();
        auto future = promise->get_future();
        
        // Store promise in pending list before submission
        {
            std::unique_lock<std::mutex> lock(pending_promises_mutex_);
            pending_promises_.emplace_back(
                new TypedPromiseHolder<void>(promise));
        }
        
        auto wrapped_task = [promise, func = std::forward<F>(f)]() {
            try {
                func();
                promise->set_value();
            } catch (...) {
                try {
                    promise->set_exception(std::current_exception());
                } catch (...) {
                    // Ignore set_exception errors
                }
            }
        };
        
        return submit_task(std::move(wrapped_task), std::move(future));
    }

    template<typename F>
    auto submit_impl(F&& f, std::false_type) -> std::future<decltype(f())> {
        using ReturnType = decltype(f());
        auto promise = std::make_shared<std::promise<ReturnType>>();
        auto future = promise->get_future();
        
        // Store promise in pending list before submission
        {
            std::unique_lock<std::mutex> lock(pending_promises_mutex_);
            pending_promises_.emplace_back(
                new TypedPromiseHolder<ReturnType>(promise));
        }
        
        auto wrapped_task = [promise, func = std::forward<F>(f)]() {
            try {
                promise->set_value(func());
            } catch (...) {
                try {
                    promise->set_exception(std::current_exception());
                } catch (...) {
                    // Ignore set_exception errors
                }
            }
        };
        
        return submit_task(std::move(wrapped_task), std::move(future));
    }

    template<typename F>
    auto submit(F&& f) -> std::future<decltype(f())> {
        if (!running_.load(std::memory_order_acquire)) {
            throw std::runtime_error("ThreadPool is shutdown");
        }

        using ReturnType = decltype(f());
        return submit_impl(std::forward<F>(f), std::is_void<ReturnType>());
    }

private:
    template<typename Task, typename Future>
    Future submit_task(Task&& task, Future&& future) {
        if (!running_.load(std::memory_order_acquire)) {
            throw std::runtime_error("ThreadPool is shutdown");
        }

        // Distribute tasks more evenly with retry logic
        for (int attempt = 0; attempt < 3; ++attempt) {
            try {
                size_t min_index = 0;
                size_t min_size = workers_[0]->local_queue.size();
                for (size_t i = 1; i < workers_.size(); ++i) {
                    size_t curr_size = workers_[i]->local_queue.size();
                    if (curr_size < min_size) {
                        min_size = curr_size;
                        min_index = i;
                    }
                }
                
                try {
                    workers_[min_index]->local_queue.push(std::forward<Task>(task));
                    active_tasks_.fetch_add(1, std::memory_order_release);
                    std::cerr << "Task submitted to worker " << min_index << "\n";
                    return std::move(future);
                } catch (const std::exception& e) {
                    std::cerr << "Failed to submit task to worker " << min_index 
                              << ": " << e.what() << "\n";
                    active_tasks_.fetch_sub(1, std::memory_order_release);
                    throw;
                }
            } catch (const std::exception& e) {
                std::cerr << "Task submission attempt " << attempt << " failed: " 
                          << e.what() << "\n";
                if (attempt == 2) throw;
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        
        throw std::runtime_error("Failed to submit task after multiple attempts");
    }

public:
    bool running() const {
        return running_.load(std::memory_order_acquire);
    }
    
    void wait();
    void shutdown();

private:
    
    void worker_loop(size_t worker_id);
    bool steal_task(Task& task, size_t thief_id);
    size_t select_victim(size_t thief_id);
};

} // namespace lockfree

#include "thread_pool.ipp"

#endif // LOCKFREE_THREAD_POOL_HPP
