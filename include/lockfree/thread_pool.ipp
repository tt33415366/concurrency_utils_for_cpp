#ifndef LOCKFREE_THREAD_POOL_IPP
#define LOCKFREE_THREAD_POOL_IPP

#include <chrono>
#include <iostream>
#include <random>

namespace lockfree {

ThreadPool::ThreadPool(size_t num_threads) {
    workers_.reserve(num_threads);
    for (size_t i = 0; i < num_threads; ++i) {
        workers_.emplace_back(new Worker());
        workers_.back()->thread = std::thread(&ThreadPool::worker_loop, this, i);
    }
}

ThreadPool::~ThreadPool() {
    if (running_.load(std::memory_order_relaxed)) {
        shutdown();
    }
}

void ThreadPool::worker_loop(size_t worker_id) {
    Worker* self = workers_[worker_id].get();
    if (!self) {
        std::cerr << "Error: Worker " << worker_id << " is null\n";
        return;
    }

    std::random_device rd;
    std::mt19937 gen(rd());

    while (running_.load(std::memory_order_acquire)) {
        if (!self) {
            std::cerr << "Error: Worker " << worker_id << " became null\n";
            break;
        }
        Task task;
        
        // 1. Try to get task from local queue
        if (self->local_queue.pop(task)) {
            self->idle.store(false, std::memory_order_relaxed);
            if (!task) continue;
            try {
                task();
            } catch (...) {
                // Swallow any exceptions during shutdown
            }
            active_tasks_.fetch_sub(1, std::memory_order_release);
            completed_tasks_.fetch_add(1, std::memory_order_release);
            continue;
        }

        // 2. Try to steal task from other workers
        if (steal_task(task, worker_id)) {
            self->idle.store(false, std::memory_order_relaxed);
            task();
            active_tasks_.fetch_sub(1, std::memory_order_release);
            continue;
        }

        // 3. No tasks available, go idle
        self->idle.store(true, std::memory_order_relaxed);
        idle_workers_.fetch_add(1, std::memory_order_relaxed);
        std::this_thread::sleep_for(std::chrono::microseconds(100));
        idle_workers_.fetch_sub(1, std::memory_order_relaxed);
    }
}

bool ThreadPool::steal_task(Task& task, size_t thief_id) {
    size_t victim = select_victim(thief_id);
    if (victim == thief_id) return false;
    
    return workers_[victim]->local_queue.pop(task);
}

size_t ThreadPool::select_victim(size_t thief_id) {
    static thread_local std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<size_t> dist(0, workers_.size() - 1);
    
    size_t victim = dist(gen);
    while (victim == thief_id && workers_.size() > 1) {
        victim = dist(gen);
    }
    return victim;
}

void ThreadPool::wait() {
    // Check both active tasks and worker queues
    while (true) {
        bool all_idle = true;
        int active = active_tasks_.load(std::memory_order_acquire);
        
        // Check if all workers are idle and queues are empty
        for (auto& worker : workers_) {
            if (!worker->idle.load(std::memory_order_relaxed) || 
                !worker->local_queue.empty()) {
                all_idle = false;
                break;
            }
        }
        
        if (active == 0 && all_idle) {
            return;
        }
        
        // Also check global queue if using one
        if (active == 0 && global_queue_.empty()) {
            return;
        }
        
        std::this_thread::yield();
    }
}

void ThreadPool::shutdown() {
    if (!running_.exchange(false, std::memory_order_release)) {
        return;
    }

    // 1. First drain all queues to prevent new tasks
    Task task;
    for (auto& worker : workers_) {
        while (worker->local_queue.pop(task)) {
            if (task) {
                task();
                active_tasks_.fetch_sub(1, std::memory_order_release);
            }
        }
    }

    // 2. Wait for active tasks to complete with timeout
    auto start = std::chrono::steady_clock::now();
    while (active_tasks_.load(std::memory_order_acquire) > 0) {
        if (std::chrono::steady_clock::now() - start > std::chrono::seconds(1)) {
            std::cerr << "Warning: Timeout waiting for tasks to complete\n";
            break;
        }
        std::this_thread::yield();
    }

    // 3. Send termination signals
    for (auto& worker : workers_) {
        worker->local_queue.push(nullptr);
    }

    // 4. Join all threads
    for (auto& worker : workers_) {
        if (worker->thread.joinable()) {
            worker->thread.join();
        }
    }
}

} // namespace lockfree

#endif // LOCKFREE_THREAD_POOL_IPP
