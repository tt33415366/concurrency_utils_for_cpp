#include <benchmark/benchmark.h>
#include "../include/lockfree/queue.hpp"
#include <queue>
#include <mutex>
#include <thread>
#include <vector>
#include <atomic>

// Producer-consumer throughput benchmarks
static void BM_LockfreeQueue_ProducerConsumer(benchmark::State& state) {
    lockfree::Queue<int> queue;
    const int producers = state.range(0);
    const int consumers = state.range(1);
    const int ops_per_thread = 100000;
    std::atomic<bool> running{true};
    std::atomic<int> total_ops{0};

    // Producer threads
    std::vector<std::thread> producer_threads;
    for (int i = 0; i < producers; ++i) {
        producer_threads.emplace_back([&] {
            for (int j = 0; j < ops_per_thread && running; ++j) {
                queue.push(j);
                total_ops.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    // Consumer threads
    std::vector<std::thread> consumer_threads;
    for (int i = 0; i < consumers; ++i) {
        consumer_threads.emplace_back([&] {
            int val;
            while (running) {
                if (queue.pop(val)) {
                    total_ops.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }

    // Run benchmark
    for (auto _ : state) {
        benchmark::DoNotOptimize(total_ops.load());
    }

    // Cleanup
    running = false;
    for (auto& t : producer_threads) t.join();
    for (auto& t : consumer_threads) t.join();

    state.SetItemsProcessed(total_ops.load());
}
BENCHMARK(BM_LockfreeQueue_ProducerConsumer)
    ->Args({1, 1})->Args({2, 2})->Args({4, 4})->Args({8, 8})
    ->Args({1, 4})->Args({4, 1}); // Different producer/consumer ratios

// Latency under contention
static void BM_LockfreeQueue_Latency(benchmark::State& state) {
    lockfree::Queue<int> queue;
    const int num_threads = state.range(0);
    std::atomic<bool> running{true};
    std::atomic<int> ready_threads{0};
    std::vector<double> latencies;

    // Worker threads
    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&] {
            ready_threads++;
            while (ready_threads < num_threads) {} // Sync start
            
            for (int j = 0; j < 10000 && running; ++j) {
                auto start = std::chrono::high_resolution_clock::now();
                queue.push(j);
                int val;
                queue.pop(val);
                auto end = std::chrono::high_resolution_clock::now();
                
                std::chrono::duration<double> elapsed = end - start;
                latencies.push_back(elapsed.count());
            }
        });
    }

    // Run benchmark
    for (auto _ : state) {
        benchmark::DoNotOptimize(latencies.size());
    }

    // Cleanup
    running = false;
    for (auto& t : threads) t.join();

    // Calculate statistics
    double sum = 0;
    for (double l : latencies) sum += l;
    state.counters["avg_latency_ns"] = (sum / latencies.size()) * 1e9;
}
BENCHMARK(BM_LockfreeQueue_Latency)
    ->Arg(2)->Arg(4)->Arg(8)->Arg(16);

BENCHMARK_MAIN();
