#include <gtest/gtest.h>
#include "../include/lockfree/queue.hpp"
#include <thread>
#include <vector>
#include <atomic>

TEST(QueueTest, BasicOperations) {
    lockfree::Queue<int> q;
    EXPECT_TRUE(q.empty());
    EXPECT_EQ(0, q.size());
    
    q.push(42);
    EXPECT_FALSE(q.empty());
    EXPECT_EQ(1, q.size());
    
    int val;
    EXPECT_TRUE(q.pop(val));
    EXPECT_EQ(42, val);
    EXPECT_TRUE(q.empty());
    EXPECT_EQ(0, q.size());
    
    // Test empty pop
    EXPECT_FALSE(q.pop(val));
}

TEST(QueueTest, ConcurrentPushPop) {
    lockfree::Queue<int> q;
    constexpr int kThreads = 4;
    constexpr int kItems = 10000;
    std::atomic<int> count{0};
    
    auto producer = [&q, &count] {
        for (int i = 0; i < kItems; ++i) {
            q.push(i);
            count.fetch_add(1, std::memory_order_relaxed);
        }
    };
    
    auto consumer = [&q, &count] {
        int val;
        while (count.load(std::memory_order_relaxed) > 0 || !q.empty()) {
            if (q.pop(val)) {
                count.fetch_sub(1, std::memory_order_relaxed);
            }
        }
    };
    
    std::vector<std::thread> threads;
    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back(producer);
        threads.emplace_back(consumer);
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_TRUE(q.empty());
    EXPECT_EQ(0, count.load());
}

TEST(QueueTest, MemoryLeakCheck) {
    // Run in valgrind or similar to check for leaks
    lockfree::Queue<int> q;
    for (int i = 0; i < 1000; ++i) {
        q.push(i);
        int val;
        q.pop(val);
    }
}

TEST(QueueTest, ABAProtection) {
    lockfree::Queue<int> q;
    
    // Simulate ABA scenario
    q.push(1);
    int val;
    q.pop(val);  // A
    
    q.push(2);   // B
    q.pop(val);  // A again
    
    // Should not corrupt the queue
    q.push(3);
    EXPECT_TRUE(q.pop(val));
    EXPECT_EQ(3, val);
}

TEST(QueueTest, MixedOperations) {
    lockfree::Queue<int> q;
    constexpr int kThreads = 4;
    constexpr int kOps = 10000;
    std::atomic<int> total{0};
    
    auto worker = [&q, &total] {
        for (int i = 0; i < kOps; ++i) {
            if (i % 3 == 0) {
                // 1/3 pops
                int val;
                if (q.pop(val)) {
                    total.fetch_sub(1, std::memory_order_relaxed);
                }
            } else {
                // 2/3 pushes
                q.push(i);
                total.fetch_add(1, std::memory_order_relaxed);
            }
        }
    };
    
    std::vector<std::thread> threads;
    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back(worker);
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    // Drain the queue
    int val;
    while (q.pop(val)) {
        total.fetch_sub(1, std::memory_order_relaxed);
    }
    
    EXPECT_EQ(0, total.load());
}

TEST(QueueTest, ExceptionSafety) {
    struct TestException {};
    lockfree::Queue<int> q;
    
    // Test push exception safety
    try {
        q.push(1);
        throw TestException();
    } catch (const TestException&) {
        int val;
        EXPECT_TRUE(q.pop(val));
        EXPECT_EQ(1, val);
    }
    
    // Test pop exception safety
    q.push(2);
    try {
        int val;
        q.pop(val);
        throw TestException();
    } catch (const TestException&) {
        EXPECT_TRUE(q.empty());
    }
}
