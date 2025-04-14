#include "../include/lockfree/thread_pool.hpp"
#include <vector>
#include <future>
#include <memory>
#include <chrono>
#include <iostream>

int main() {
    const int num_threads = 4;
    const int tasks_per_thread = 100000;
    lockfree::ThreadPool pool(num_threads);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    std::vector<std::future<int>> futures;
    futures.reserve(num_threads * tasks_per_thread);
    
    for (int i = 0; i < num_threads * tasks_per_thread; ++i) {
        futures.push_back(pool.submit([i]{
            return i; // Return task index
        }));
    }
    
    for (auto& f : futures) {
        f.get();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    std::cout << "Processed " << (num_threads * tasks_per_thread) 
              << " tasks in " << duration << "ms\n";
    std::cout << "Throughput: " 
              << (num_threads * tasks_per_thread * 1000 / duration) 
              << " tasks/second\n";
              
    return 0;
}
