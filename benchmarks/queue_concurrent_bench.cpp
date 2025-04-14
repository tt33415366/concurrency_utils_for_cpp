#include <benchmark/benchmark.h>
#include "../include/lockfree/queue.hpp"
#include <queue>
#include <mutex>
#include <thread>
#include <vector>

static void BM_LockfreeQueue_Concurrent(benchmark::State& state) {
    lockfree::Queue<int> queue;
    const int num_threads = state.range(0);
    const int ops_per_thread = 10000;
    
    for (auto _ : state) {
        std::vector<std::thread> threads;
        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back([&] {
                for (int j = 0; j < ops_per_thread; ++j) {
                    queue.push(j);
                    int val;
                    queue.pop(val);
                }
            });
        }
        
        for (auto& t : threads) {
            t.join();
        }
    }
    
    state.SetItemsProcessed(state.iterations() * num_threads * ops_per_thread * 2);
}
BENCHMARK(BM_LockfreeQueue_Concurrent)
    ->Arg(2)->Arg(4)->Arg(8)->Arg(16);

static void BM_MutexQueue_Concurrent(benchmark::State& state) {
    std::queue<int> queue;
    std::mutex mtx;
    const int num_threads = state.range(0);
    const int ops_per_thread = 10000;
    
    for (auto _ : state) {
        std::vector<std::thread> threads;
        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back([&] {
                for (int j = 0; j < ops_per_thread; ++j) {
                    {
                        std::lock_guard<std::mutex> lock(mtx);
                        queue.push(j);
                    }
                    {
                        std::lock_guard<std::mutex> lock(mtx);
                        if (!queue.empty()) {
                            queue.pop();
                        }
                    }
                }
            });
        }
        
        for (auto& t : threads) {
            t.join();
        }
    }
    
    state.SetItemsProcessed(state.iterations() * num_threads * ops_per_thread * 2);
}
BENCHMARK(BM_MutexQueue_Concurrent)
    ->Arg(2)->Arg(4)->Arg(8)->Arg(16);

BENCHMARK_MAIN();
