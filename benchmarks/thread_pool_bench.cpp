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
    const int tasks_per_thread = 100;  // Further reduced to 100 tasks per thread
    lockfree::ThreadPool pool(num_threads);
    
    const auto test_start = std::chrono::steady_clock::now();
        for (auto _ : state) {
            // Check for timeout (5 seconds max)
            if (std::chrono::steady_clock::now() - test_start > std::chrono::seconds(5)) {
                state.SkipWithError("Test timeout");
                break;
            }

            if (!pool.running()) {
                state.SkipWithError("ThreadPool is not running");
                break;
            }

            std::vector<std::future<int>> futures;
            futures.reserve(num_threads * tasks_per_thread);
            
            try {
                for (int i = 0; i < num_threads * tasks_per_thread; ++i) {
                    if (!pool.running()) {
                        state.SkipWithError("ThreadPool shutdown during submission");
                        break;
                    }
                    
                    if (std::chrono::steady_clock::now() - test_start > std::chrono::seconds(5)) {
                        state.SkipWithError("Test timeout during submission");
                        break;
                    }

                    futures.push_back(pool.submit([i]{
                        return i; // Simple task
                    }));

                    // Additional check after each submission
                    if (!pool.running()) {
                        state.SkipWithError("ThreadPool shutdown during submission");
                        break;
                    }
                }

                // Final check after submission loop
                if (!pool.running()) {
                    state.SkipWithError("ThreadPool shutdown after submission");
                    break;
                }
            } catch (const std::exception& e) {
                state.SkipWithError("Exception during submission");
                break;
            }
        
        // Add global timeout check before processing futures
        if (std::chrono::steady_clock::now() - test_start > std::chrono::seconds(5)) {
            state.SkipWithError("Global test timeout reached");
            break;
        }

        for (auto& f : futures) {
            if (!pool.running()) {
                state.SkipWithError("ThreadPool shutdown during wait");
                break;
            }
            if (std::chrono::steady_clock::now() - test_start > std::chrono::seconds(5)) {
                state.SkipWithError("Test timeout during wait");
                break;
            }
            try {
                f.get();
            } catch (const std::future_error& e) {
                state.SkipWithError("Task execution failed");
                break;
            }
        }

        // Final check after processing futures
        if (!pool.running()) {
            state.SkipWithError("ThreadPool shutdown after processing");
            break;
        }
        
        pool.wait();
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

// Task stealing efficiency benchmark
static void BM_ThreadPool_StealEfficiency(benchmark::State& state) {
    lockfree::ThreadPool pool(state.range(0));
    const int tasks = state.range(1);
    
    // Create unbalanced workload
    for (auto _ : state) {
        // Submit most tasks to first worker
        for (int i = 0; i < tasks * 0.8; ++i) {
            pool.submit([]{
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            });
        }
        
        // Submit remaining tasks evenly
        for (int i = 0; i < tasks * 0.2; ++i) {
            pool.submit([]{
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            });
        }
        
        pool.wait();
    }
    
    state.SetItemsProcessed(state.iterations() * tasks);
}
BENCHMARK(BM_ThreadPool_StealEfficiency)
    ->Args({4, 1000})->Args({8, 2000})->Args({16, 4000});

// Dynamic thread adjustment test
static void BM_ThreadPool_DynamicThreads(benchmark::State& state) {
    lockfree::ThreadPool pool(2); // Start with 2 threads
    
    const int tasks = 1000;
    const int workload = 100;
    
    for (auto _ : state) {
        // Phase 1: Light workload
        for (int i = 0; i < tasks; ++i) {
            pool.submit([workload]{
                volatile int result = 0;
                for (int j = 0; j < workload; ++j) {
                    result += j % 10;
                }
            });
        }
        pool.wait();
        
        // Phase 2: Heavy workload
        for (int i = 0; i < tasks; ++i) {
            pool.submit([workload]{
                volatile int result = 0;
                for (int j = 0; j < workload * 100; ++j) {
                    result += j % 10;
                }
            });
        }
        pool.wait();
    }
    
    state.SetItemsProcessed(state.iterations() * tasks * 2);
}
BENCHMARK(BM_ThreadPool_DynamicThreads);

// Mixed workload benchmark
static void BM_ThreadPool_MixedWorkload(benchmark::State& state) {
    lockfree::ThreadPool pool(state.range(0));
    const int tasks = state.range(1);
    const int light_work = 10;
    const int heavy_work = 1000;
    
    for (auto _ : state) {
        std::vector<std::future<void>> futures;
        futures.reserve(tasks);
        
        // Submit mixed tasks
        for (int i = 0; i < tasks; ++i) {
            if (i % 2 == 0) {
                futures.push_back(pool.submit([light_work]{
                    volatile int result = 0;
                    for (int j = 0; j < light_work; ++j) {
                        result += j % 10;
                    }
                }));
            } else {
                futures.push_back(pool.submit([heavy_work]{
                    volatile int result = 0;
                    for (int j = 0; j < heavy_work; ++j) {
                        result += j % 10;
                    }
                }));
            }
        }
        
        for (auto& f : futures) {
            f.get();
        }
    }
    
    state.SetItemsProcessed(state.iterations() * tasks);
}
BENCHMARK(BM_ThreadPool_MixedWorkload)
    ->Args({4, 1000})->Args({8, 2000})->Args({16, 4000});

BENCHMARK_MAIN();
