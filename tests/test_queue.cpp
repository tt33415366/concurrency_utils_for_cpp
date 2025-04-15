#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <atomic>
#include "../include/lockfree/queue.hpp"

using namespace lockfree;

TEST(LockfreeQueueTest, Basic) {
    lockfree::Queue<int> q;
    EXPECT_TRUE(q.empty());
    
    q.push(42);
    EXPECT_FALSE(q.empty());
    
    int val;
    EXPECT_TRUE(q.pop(val));
    EXPECT_EQ(42, val);
    EXPECT_TRUE(q.empty());
}

TEST(LockfreeQueueTest, ConcurrentPush) {
    lockfree::Queue<int> q;
    constexpr int kThreads = 4;
    constexpr int kPerThread = 1000;
    std::vector<std::thread> threads;
    
    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back([&q] {
            for (int j = 0; j < kPerThread; ++j) {
                q.push(j);
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_EQ(kThreads * kPerThread, q.size());
}

TEST(LockfreeQueueTest, ConcurrentPop) {
    lockfree::Queue<int> q;
    constexpr int kItems = 10000;
    for (int i = 0; i < kItems; ++i) {
        q.push(i);
    }
    
    std::atomic<int> count{0};
    std::vector<std::thread> threads;
    constexpr int kThreads = 4;
    
    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back([&q, &count] {
            int val;
            while (q.pop(val)) {
                ++count;
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_EQ(kItems, count);
    EXPECT_TRUE(q.empty());
}
