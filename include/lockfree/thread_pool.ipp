#ifndef LOCKFREE_THREAD_POOL_IPP
#define LOCKFREE_THREAD_POOL_IPP

#include <chrono>
#include <iostream>
#include <random>

namespace lockfree {

ThreadPool::ThreadPool(size_t num_threads) {
    std::cerr << "ThreadPool constructor called with " << num_threads << " threads\n";
    
    // Initialize workers vector atomically
    std::vector<std::shared_ptr<Worker>> temp_workers;
    temp_workers.reserve(num_threads);
    for (size_t i = 0; i < num_threads; ++i) {
        temp_workers.emplace_back(std::make_shared<Worker>());
        std::cerr << "Worker " << i << " created\n";
    }
    
    // Atomically swap the initialized workers
    workers_.swap(temp_workers);
    
    // Start worker threads
    std::atomic<size_t> threads_started{0};
    for (size_t i = 0; i < num_threads; ++i) {
        try {
            workers_[i]->thread = std::thread([this, i, &threads_started]() {
                workers_[i]->idle.store(false, std::memory_order_relaxed);
                threads_started.fetch_add(1, std::memory_order_release);
                this->worker_loop(i);
            });
            
            // Wait for thread to start
            while (threads_started.load(std::memory_order_acquire) <= i) {
                std::this_thread::yield();
            }
            std::cerr << "Worker " << i << " thread started\n";
        } catch (...) {
            running_.store(false, std::memory_order_release);
            throw;
        }
    }
    std::cerr << "ThreadPool constructor completed\n";
}

void ThreadPool::worker_loop(size_t worker_id) {
    std::cerr << "Worker " << worker_id << " starting\n";
    
    // Safely get worker pointer
    if (worker_id >= workers_.size() || !workers_[worker_id]) {
        return;
    }
    Worker* self = workers_[worker_id].get();
    
    // Verify worker is valid
    if (!self || !self->is_valid()) {
        return;
    }

    size_t loop_count = 0;
    tasks_executed_.store(0, std::memory_order_relaxed);
    const auto start_time = std::chrono::steady_clock::now();
    while (running_.load(std::memory_order_acquire)) {
        // Check if pool is shutting down
        if (!running_.load(std::memory_order_acquire)) {
            break;
        }

        std::atomic_thread_fence(std::memory_order_seq_cst);

        loop_count++;
        if (loop_count % 500 == 0) {  // More frequent debugging
            std::cerr << "Worker " << worker_id << " alive (loop " << loop_count 
                      << ") queue size: " << self->local_queue.size() 
                      << " active: " << active_tasks_.load() << "\n";
        }

        Task local_task;
        if (self->local_queue.pop(local_task)) {
            // Check for shutdown signal (null task)
            if (!local_task) {
                break;
            }
            if (local_task) {  // Check if task is valid
                try {
                    std::cerr << "Worker " << worker_id << " executing task\n";
                    local_task(); 
                    tasks_executed_.fetch_add(1, std::memory_order_relaxed);
                    std::cerr << "Worker " << worker_id << " task completed\n";
                } catch (const std::bad_function_call& e) {
                    std::cerr << "Worker " << worker_id << " bad task: " << e.what() << "\n";
                    active_tasks_.fetch_sub(1, std::memory_order_release);
                } catch (const std::exception& e) {
                    std::cerr << "Worker " << worker_id << " task exception: " << e.what() << "\n";
                } catch (...) {
                    std::cerr << "Worker " << worker_id << " unknown task exception\n";
                }
            } else {
                std::cerr << "Worker " << worker_id << " got null task\n";
                active_tasks_.fetch_sub(1, std::memory_order_release);
            }
            active_tasks_.fetch_sub(1, std::memory_order_release);
            continue;
        }

        // Check global queue before stealing
        Task global_task;
        if (global_queue_.pop(global_task)) {
            if (global_task) {
                try {
                    std::cerr << "Worker " << worker_id << " executing global task\n";
                    global_task();
                    tasks_executed_.fetch_add(1, std::memory_order_relaxed);
                    std::cerr << "Worker " << worker_id << " global task completed\n";
                    active_tasks_.fetch_sub(1, std::memory_order_release);
                    continue;
                } catch (...) {
                    active_tasks_.fetch_sub(1, std::memory_order_release);
                }
            }
        }

        // Attempt to steal work from another worker
        Task stolen_task;
        if (steal_task(stolen_task, worker_id)) {
            if (stolen_task) {
                try {
                    std::cerr << "Worker " << worker_id << " executing stolen task\n";
                    stolen_task();
                    tasks_executed_.fetch_add(1, std::memory_order_relaxed);
                    std::cerr << "Worker " << worker_id << " stolen task completed\n";
                    active_tasks_.fetch_sub(1, std::memory_order_release);
                    continue;
                } catch (...) {
                    active_tasks_.fetch_sub(1, std::memory_order_release);
                }
            }
        }

        // Yield to avoid busy waiting
        std::this_thread::yield();
    }
}

bool ThreadPool::steal_task(Task& task, size_t thief_id) {
    size_t victim = select_victim(thief_id);
    if (victim == thief_id || victim >= workers_.size()) {
        return false;
    }
    return workers_[victim]->local_queue.pop(task);
}

size_t ThreadPool::select_victim(size_t thief_id) {
    static thread_local std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<size_t> dist(0, workers_.size() - 1);
    return dist(gen);
}

void ThreadPool::wait() {
    const auto start = std::chrono::steady_clock::now();
    while (active_tasks_.load(std::memory_order_acquire) > 0) {
        if (std::chrono::steady_clock::now() - start > std::chrono::seconds(10)) {
            std::cerr << "Warning: wait() timeout after 10 seconds\n";
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void ThreadPool::shutdown() {
    running_.store(false, std::memory_order_release);
    
    // Send null tasks to wake up all workers
    for (auto& worker : workers_) {
        if (worker) {
            worker->local_queue.push(nullptr);
        }
    }
    
    // Join threads with timeout
    for (auto& worker : workers_) {
        if (worker && worker->thread.joinable()) {
            if (worker->thread.joinable()) {
                worker->thread.join();
            }
        }
    }
    
    std::cerr << "ThreadPool shutdown completed\n";
}

} // namespace lockfree

#endif // LOCKFREE_THREAD_POOL_IPP
