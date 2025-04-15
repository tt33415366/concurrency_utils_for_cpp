#include <benchmark/benchmark.h>
#include "../include/lockfree/queue.hpp"
#include <queue>
#include <mutex>
#include <atomic>
#include <thread>
#include <vector>

// Single-threaded benchmarks
static void BM_LockfreeQueue_Throughput(benchmark::State& state) {
    lockfree::Queue<int> queue;
    for (auto _ : state) {
        queue.push(1);
        int val;
        queue.pop(val);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LockfreeQueue_Throughput);

static void BM_MutexQueue_Throughput(benchmark::State& state) {
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
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_MutexQueue_Throughput);

// Multi-threaded benchmarks
static void BM_LockfreeQueue_Contention(benchmark::State& state) {
    lockfree::Queue<int> queue;
    std::atomic<bool> running{true};
    std::atomic<int> total_ops{0};

    // Producer thread
    auto producer = [&] {
        while (running) {
            queue.push(1);
            total_ops.fetch_add(1, std::memory_order_relaxed);
        }
    };

    // Consumer thread
    auto consumer = [&] {
        int val;
        while (running) {
            if (queue.pop(val)) {
                total_ops.fetch_add(1, std::memory_order_relaxed);
            }
        }
    };

    // Start threads
    std::vector<std::thread> threads;
    threads.emplace_back(producer);
    threads.emplace_back(consumer);

    // Run benchmark
    for (auto _ : state) {
        benchmark::DoNotOptimize(total_ops.load());
    }

    // Cleanup
    running = false;
    for (auto& t : threads) {
        t.join();
    }

    state.SetItemsProcessed(total_ops.load());
}
BENCHMARK(BM_LockfreeQueue_Contention)->Threads(2);

static void BM_MutexQueue_Contention(benchmark::State& state) {
    std::queue<int> queue;
    std::mutex mtx;
    std::atomic<bool> running{true};
    std::atomic<int> total_ops{0};

    // Producer thread
    auto producer = [&] {
        while (running) {
            {
                std::lock_guard<std::mutex> lock(mtx);
                queue.push(1);
            }
            total_ops.fetch_add(1, std::memory_order_relaxed);
        }
    };

    // Consumer thread
    auto consumer = [&] {
        while (running) {
            {
                std::lock_guard<std::mutex> lock(mtx);
                if (!queue.empty()) {
                    queue.pop();
                    total_ops.fetch_add(1, std::memory_order_relaxed);
                }
            }
        }
    };

    // Start threads
    std::vector<std::thread> threads;
    threads.emplace_back(producer);
    threads.emplace_back(consumer);

    // Run benchmark
    for (auto _ : state) {
        benchmark::DoNotOptimize(total_ops.load());
    }

    // Cleanup
    running = false;
    for (auto& t : threads) {
        t.join();
    }

    state.SetItemsProcessed(total_ops.load());
}
BENCHMARK(BM_MutexQueue_Contention)->Threads(2);

// Latency benchmarks
static void BM_LockfreeQueue_Latency(benchmark::State& state) {
    lockfree::Queue<int> queue;
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        queue.push(1);
        int val;
        queue.pop(val);
        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        state.SetIterationTime(elapsed.count() / 1e9);
    }
}
BENCHMARK(BM_LockfreeQueue_Latency)->UseManualTime();

BENCHMARK_MAIN();
