#include <gtest/gtest.h>
#include "../include/lockfree/aba_protected_queue.hpp"
#include <thread>
#include <vector>
#include <atomic>

class ABAProtectedQueueTest : public ::testing::Test {
protected:
    lockfree::ABAProtectedQueue<int> q;
};

TEST_F(ABAProtectedQueueTest, BasicOperations) {
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
}

TEST_F(ABAProtectedQueueTest, ConcurrentOperations) {
    constexpr int kThreads = 4;
    constexpr int kItems = 1000;
    std::atomic<int> count{0};
    
    auto producer = [this, &count] {
        for (int i = 0; i < kItems; ++i) {
            q.push(i);
            count.fetch_add(1, std::memory_order_relaxed);
        }
    };
    
    auto consumer = [this, &count] {
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