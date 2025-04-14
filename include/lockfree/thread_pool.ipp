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
    while (!task_queue_.empty()) {
        std::this_thread::yield();
    }
}

void ThreadPool::shutdown() {
    bool expected = true;
    if (!running_.compare_exchange_strong(expected, false)) {
        return; // Already shutdown
    }

    // Wake up all threads with proper synchronization
    std::atomic<size_t> count{workers_.size()};
    for (size_t i = 0; i < workers_.size(); ++i) {
        task_queue_.push([&count]{
            count.fetch_sub(1, std::memory_order_release);
        });
    }

    // Wait for all workers to finish
    while (count.load(std::memory_order_acquire) > 0) {
        std::this_thread::yield();
    }

    // Join all threads
    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

void ThreadPool::worker_loop() {
    while (running_.load(std::memory_order_acquire)) {
        Task task;
        if (task_queue_.pop(task)) {
            try {
                task();
            } catch (...) {
                // Swallow exceptions to prevent thread termination
            }
        } else {
            std::this_thread::yield();
        }
    }

    // Drain remaining tasks after shutdown
    Task task;
    while (task_queue_.pop(task)) {
        try {
            task();
        } catch (...) {
            // Swallow exceptions
        }
    }
}

} // namespace lockfree

#endif // LOCKFREE_THREAD_POOL_IPP
