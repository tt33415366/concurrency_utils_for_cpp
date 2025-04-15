#include <benchmark/benchmark.h>
#include "../include/lockfree/thread_pool.hpp"
#include <vector>
#include <future>
#include <memory>
#include <thread>
#include <chrono>
#include <numeric>

// Throughput benchmarks
static void BM_ThreadPool_Throughput(benchmark::State& state) {
    const int num_threads = state.range(0);
    const int tasks_per_thread = 10000;
    lockfree::ThreadPool pool(num_threads);
    
    for (auto _ : state) {
        std::vector<std::future<int>> futures;
        futures.reserve(num_threads * tasks_per_thread);
        
        for (int i = 0; i < num_threads * tasks_per_thread; ++i) {
            futures.push_back(pool.submit([i]{
                return i; // Simple task
            }));
        }
        
        for (auto& f : futures) {
            f.get();
        }
    }
    
    state.SetItemsProcessed(state.iterations() * num_threads * tasks_per_thread);
}
BENCHMARK(BM_ThreadPool_Throughput)
    ->Arg(2)->Arg(4)->Arg(8)->Arg(16);

// Latency benchmarks
static void BM_ThreadPool_Latency(benchmark::State& state) {
    lockfree::ThreadPool pool(state.range(0));
    const int tasks = 1000;
    std::vector<double> latencies;
    latencies.reserve(tasks);

    for (auto _ : state) {
        for (int i = 0; i < tasks; ++i) {
            auto start = std::chrono::high_resolution_clock::now();
            auto future = pool.submit([&] {
                return 0; // Simple task
            });
            future.get();
            auto end = std::chrono::high_resolution_clock::now();
            
            std::chrono::duration<double> elapsed = end - start;
            latencies.push_back(elapsed.count());
        }
    }

    // Calculate average latency
    double sum = std::accumulate(latencies.begin(), latencies.end(), 0.0);
    state.counters["avg_latency_ns"] = (sum / latencies.size()) * 1e9;
}
BENCHMARK(BM_ThreadPool_Latency)
    ->Arg(2)->Arg(4)->Arg(8)->Arg(16);

// Workload complexity benchmarks
static void BM_ThreadPool_Workload(benchmark::State& state) {
    lockfree::ThreadPool pool(state.range(0));
    const int tasks = 1000;
    const int workload = state.range(1); // Workload complexity factor

    for (auto _ : state) {
        std::vector<std::future<int>> futures;
        futures.reserve(tasks);
        
        for (int i = 0; i < tasks; ++i) {
            futures.push_back(pool.submit([workload]{
                // Simulate workload
                int result = 0;
                for (int j = 0; j < workload * 1000; ++j) {
                    result += j % 10;
                }
                return result;
            }));
        }
        
        for (auto& f : futures) {
            f.get();
        }
    }

    state.SetItemsProcessed(state.iterations() * tasks);
}
BENCHMARK(BM_ThreadPool_Workload)
    ->Args({2, 1})->Args({2, 10})->Args({2, 100})  // Varying workloads
    ->Args({4, 1})->Args({4, 10})->Args({4, 100})
    ->Args({8, 1})->Args({8, 10})->Args({8, 100});

// Comparison against std::thread
static void BM_StdThread_Throughput(benchmark::State& state) {
    const int num_threads = state.range(0);
    const int tasks_per_thread = 10000;
    
    for (auto _ : state) {
        std::vector<std::thread> threads;
        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back([tasks_per_thread] {
                for (int j = 0; j < tasks_per_thread; ++j) {
                    // Simple task
                }
            });
        }
        
        for (auto& t : threads) {
            t.join();
        }
    }
    
    state.SetItemsProcessed(state.iterations() * num_threads * tasks_per_thread);
}
BENCHMARK(BM_StdThread_Throughput)
    ->Arg(2)->Arg(4)->Arg(8)->Arg(16);

BENCHMARK_MAIN();
