#include <gtest/gtest.h>
#include "../include/lockfree/queue.hpp"

TEST(LockfreeQueueTest, InitialEmpty) {
    lockfree::Queue<int> queue;
    EXPECT_TRUE(queue.empty());
    EXPECT_EQ(queue.size(), 0);
}

TEST(LockfreeQueueTest, SinglePushPop) {
    lockfree::Queue<int> queue;
    queue.push(42);
    EXPECT_FALSE(queue.empty());
    EXPECT_EQ(queue.size(), 1);

    int value = 0;
    EXPECT_TRUE(queue.pop(value));
    EXPECT_EQ(value, 42);
    EXPECT_TRUE(queue.empty());
    EXPECT_EQ(queue.size(), 0);
}

TEST(LockfreeQueueTest, MultiplePushPop) {
    lockfree::Queue<int> queue;
    for (int i = 0; i < 10; ++i) {
        queue.push(i);
    }
    EXPECT_EQ(queue.size(), 10);

    for (int i = 0; i < 10; ++i) {
        int value = 0;
        EXPECT_TRUE(queue.pop(value));
        EXPECT_EQ(value, i);
    }
    EXPECT_TRUE(queue.empty());
}

TEST(LockfreeQueueTest, PopEmpty) {
    lockfree::Queue<int> queue;
    int value = 0;
    EXPECT_FALSE(queue.pop(value));
}
