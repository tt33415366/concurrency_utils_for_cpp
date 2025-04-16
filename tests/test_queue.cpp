#include "gtest/gtest.h"
#include "../include/lockfree/queue.hpp"

TEST(QueueTest, BasicOperations) {
    lockfree::Queue<int> q;
    EXPECT_TRUE(q.empty());
    
    q.push(42);
    EXPECT_FALSE(q.empty());
    
    int val;
    EXPECT_TRUE(q.pop(val));
    EXPECT_EQ(42, val);
    EXPECT_TRUE(q.empty());
}
