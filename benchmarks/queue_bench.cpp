#include <benchmark/benchmark.h>
#include "../include/lockfree/queue.hpp"
#include <queue>
#include <mutex>

// Lock-free queue benchmark
static void BM_LockfreeQueue(benchmark::State& state) {
    lockfree::Queue<int> queue;
    for (auto _ : state) {
        queue.push(1);
        int val;
        queue.pop(val);
    }
}
BENCHMARK(BM_LockfreeQueue);

// Mutex-protected std::queue benchmark
static void BM_MutexQueue(benchmark::State& state) {
    std::queue<int> queue;
    std::mutex mtx;
    for (auto _ : state) {
        {
            std::lock_guard<std::mutex> lock(mtx);
            queue.push(1);
        }
        {
            std::lock_guard<std::mutex> lock(mtx);
            if (!queue.empty()) {
                queue.pop();
            }
        }
    }
}
BENCHMARK(BM_MutexQueue);

BENCHMARK_MAIN();
