#include <gtest/gtest.h>
#include "../include/lockfree/queue.hpp"

namespace lfq {} // Forward declare namespace

TEST(LFQueueTest, InitialEmpty) {
    lfq::Queue<int> queue;
    EXPECT_TRUE(queue.empty());
    EXPECT_EQ(0, queue.size());
}

TEST(LFQueueTest, SinglePushPop) {
    lfq::Queue<int> queue;
    queue.push(42);
    EXPECT_FALSE(queue.empty());
    EXPECT_EQ(1, queue.size());

    int value = 0;
    EXPECT_TRUE(queue.pop(value));
    EXPECT_EQ(42, value);
    EXPECT_TRUE(queue.empty());
    EXPECT_EQ(0, queue.size());
}

TEST(LFQueueTest, MultiplePushPop) {
    lfq::Queue<int> queue;
    for (int i = 0; i < 10; ++i) {
        queue.push(i);
    }
    EXPECT_EQ(10, queue.size());

    for (int i = 0; i < 10; ++i) {
        int value = 0;
        EXPECT_TRUE(queue.pop(value));
        EXPECT_EQ(i, value);
    }
    EXPECT_TRUE(queue.empty());
}

TEST(LFQueueTest, PopEmpty) {
    lfq::Queue<int> queue;
    int value = 0;
    EXPECT_FALSE(queue.pop(value));
}
