#ifndef LOCKFREE_THREAD_POOL_IPP
#define LOCKFREE_THREAD_POOL_IPP

namespace lockfree {

ThreadPool::ThreadPool(size_t num_threads) {
    workers_.reserve(num_threads);
    for (size_t i = 0; i < num_threads; ++i) {
        workers_.emplace_back(&ThreadPool::worker_loop, this);
    }
}

ThreadPool::~ThreadPool() {
    if (running_.load(std::memory_order_relaxed)) {
        shutdown();
    }
}


void ThreadPool::wait() {
    while (true) {
        // Full memory barrier to ensure we see all updates
        std::atomic_thread_fence(std::memory_order_seq_cst);
        
        // Check both queue and active tasks with strong ordering
        bool queue_empty = task_queue_.empty();
        int active = active_tasks_.load(std::memory_order_seq_cst);
        
        if (queue_empty && active == 0) {
            break;
        }
        
        // Brief sleep to prevent busy waiting
        std::this_thread::sleep_for(std::chrono::microseconds(10));
    }
}

void ThreadPool::shutdown() {
    bool expected = true;
    if (!running_.compare_exchange_strong(expected, false)) {
        return; // Already shutdown
    }

    // First wait for active tasks to complete
    wait();

    // Wake up all threads with poison pills
    for (size_t i = 0; i < workers_.size(); ++i) {
        try {
            task_queue_.push(nullptr); // Poison pill
        } catch (...) {
            // Ignore queue errors during shutdown
        }
    }

    // Join all threads with timeout
    for (auto& worker : workers_) {
        if (worker.joinable()) {
            try {
                worker.join();
            } catch (...) {
                worker.detach(); // Force detach if join fails
            }
        }
    }

    // Final cleanup of any remaining tasks
    Task task;
    while (task_queue_.pop(task)) {
        if (task) {
            try {
                task();
            } catch (...) {}
        }
    }
}

void ThreadPool::worker_loop() {
    while (true) {
        Task task;
        
        // Try to get a task with timeout
        if (task_queue_.pop(task)) {
            if (!task) { // Poison pill received
                break;
            }
            
            try {
                task();
            } catch (...) {
                // Swallow exceptions to prevent thread termination
            }
        } 
        else if (!running_.load(std::memory_order_acquire)) {
            // Exit if queue is empty and shutdown was requested
            break;
        }
        else {
            // Reduce contention when queue is empty
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    }

    // Drain remaining tasks after shutdown
    if (running_.load(std::memory_order_acquire)) {
        Task task;
        while (task_queue_.pop(task)) {
            if (task) { // Skip poison pills
                try {
                    task();
                } catch (...) {
                    // Swallow exceptions
                }
            }
        }
    }
}

} // namespace lockfree

#endif // LOCKFREE_THREAD_POOL_IPP
