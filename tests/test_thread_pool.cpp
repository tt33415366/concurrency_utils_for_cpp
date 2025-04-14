#include <gtest/gtest.h>
#include "../include/lockfree/thread_pool.hpp"
#include <atomic>

TEST(ThreadPoolTest, BasicTaskExecution) {
    lockfree::ThreadPool pool(2);
    std::atomic_int counter(0);
    
    pool.submit([&counter]() {
        counter.fetch_add(1);
    });
    
    // Wait for task completion
    while (counter.load() == 0) {
        // Spin wait
    }
    
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

TEST(ThreadPoolTest, ShutdownBehavior) {
    lockfree::ThreadPool pool(2);
    std::atomic_int counter(0);
    
    pool.submit([&counter]() {
        counter.fetch_add(1);
    });
    
    // Wait for task completion
    while (counter.load() == 0) {
        // Spin wait
    }
    
    pool.shutdown();
    EXPECT_EQ(1, counter.load());
    
    // Verify no new tasks accepted
    bool accepted = true;
    try {
        pool.submit([&counter]() {
            counter.fetch_add(1);
        });
    } catch (...) {
        accepted = false;
    }
    EXPECT_FALSE(accepted);
}

TEST(ThreadPoolTest, ExceptionHandling) {
    lockfree::ThreadPool pool(2);
    std::atomic_bool exception_caught(false);
    
    pool.submit([]() {
        throw std::runtime_error("Test exception");
    });
    
    // Wait briefly to allow task processing
    for (int i = 0; i < 100 && !exception_caught.load(); ++i) {
        // Spin wait
    }
    
    EXPECT_FALSE(exception_caught.load());
}
