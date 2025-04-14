#include <benchmark/benchmark.h>
#include "../include/lockfree/thread_pool.hpp"
#include <vector>
#include <future>
#include <memory>

static void BM_ThreadPoolThroughput(benchmark::State& state) {
    const int num_threads = state.range(0);
    const int tasks_per_thread = 10000;
    lockfree::ThreadPool pool(num_threads);
    
    for (auto _ : state) {
        std::vector<std::future<int>> futures;
        futures.reserve(num_threads * tasks_per_thread);
        
        for (int i = 0; i < num_threads * tasks_per_thread; ++i) {
            futures.push_back(pool.submit([i]{
                // Return dummy value
                return i;
            }));
        }
        
        for (auto& f : futures) {
            f.get();
        }
    }
    
    state.SetItemsProcessed(state.iterations() * num_threads * tasks_per_thread);
}
BENCHMARK(BM_ThreadPoolThroughput)
    ->Arg(2)->Arg(4)->Arg(8)->Arg(16);

BENCHMARK_MAIN();
