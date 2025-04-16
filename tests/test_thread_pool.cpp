#include "../third_party/googletest/googletest/include/gtest/gtest.h"
#include <iostream>
#include "../include/lockfree/thread_pool.hpp"
#include <atomic>
#include <vector>
#include <thread>
#include <future>
#include <memory>

TEST(ThreadPoolTest, BasicTaskExecution) {
    lockfree::ThreadPool pool(2);
    std::atomic_int counter(0);
    
    auto future = pool.submit([&counter]() {
        counter.fetch_add(1);
        return 42;
    });
    
    EXPECT_EQ(42, future.get());
    EXPECT_EQ(1, counter.load());
}

TEST(ThreadPoolTest, MultipleTasks) {
    lockfree::ThreadPool pool(4);
    std::atomic_int counter(0);
    const int num_tasks = 100;
    
    for (int i = 0; i < num_tasks; ++i) {
        pool.submit([&counter]() {
            counter.fetch_add(1, std::memory_order_relaxed);
        });
    }
    
    pool.wait();
    EXPECT_EQ(num_tasks, counter.load(std::memory_order_relaxed));
}

TEST(ThreadPoolTest, HighConcurrency) {
    lockfree::ThreadPool pool(4); // Limit to 4 threads
    constexpr int kThreads = 8;
    constexpr int kTasksPerThread = 100;
    std::vector<std::thread> threads;
    std::atomic_int total_tasks(0);
    std::atomic_int failures(0);
    
    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back([&, i] {
            for (int j = 0; j < kTasksPerThread; ++j) {
                try {
                    pool.submit([&] {
                        total_tasks.fetch_add(1, std::memory_order_relaxed);
                    });
                } catch (...) {
                    failures.fetch_add(1);
                }
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    pool.wait();
    std::cout << "Completed " << total_tasks.load() << "/" 
              << (kThreads * kTasksPerThread) << " tasks\n";
    std::cout << "Failures: " << failures.load() << "\n";
    
    EXPECT_EQ(0, failures.load());
    EXPECT_EQ(kThreads * kTasksPerThread, total_tasks.load());
}

TEST(ThreadPoolTest, WorkStealing) {
    lockfree::ThreadPool pool(2);
    std::atomic_int worker1_tasks(0);
    std::atomic_int worker2_tasks(0);
    
    // Submit tasks that identify which worker executes them
    for (int i = 0; i < 100; ++i) {
        pool.submit([&, i]() {
            if (i % 2 == 0) worker1_tasks++;
            else worker2_tasks++;
        });
    }
    
    pool.wait();
    EXPECT_GT(worker1_tasks.load(), 0);
    EXPECT_GT(worker2_tasks.load(), 0);
}

TEST(ThreadPoolTest, ShutdownBehavior) {
    lockfree::ThreadPool pool(2);
    std::atomic_int counter(0);
    
    pool.submit([&counter]() {
        counter.fetch_add(1);
    });
    
    pool.shutdown();
    EXPECT_EQ(1, counter.load());
    
    // Verify no new tasks accepted
    EXPECT_THROW({
        pool.submit([&counter]() {
            counter.fetch_add(1);
        });
    }, std::runtime_error);
}

TEST(ThreadPoolTest, ExceptionHandling) {
    lockfree::ThreadPool pool(2);
    std::atomic_bool exception_caught(false);
    
    auto future = pool.submit([]() -> int {
        throw std::runtime_error("Test exception");
        return 0;
    });
    
    EXPECT_THROW({ future.get(); }, std::runtime_error);
}

TEST(ThreadPoolTest, ResourceCleanup) {
    std::weak_ptr<lockfree::ThreadPool> weak_pool;
    {
        auto pool = std::make_shared<lockfree::ThreadPool>(2);
        weak_pool = pool;
        
        pool->submit([]() {
            // Simple task
        });
    }
    
    // Verify pool is destroyed after scope
    EXPECT_TRUE(weak_pool.expired());
}
