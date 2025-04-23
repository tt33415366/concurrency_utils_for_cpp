#ifndef LOCKFREE_ABA_PROTECTED_QUEUE_H
#define LOCKFREE_ABA_PROTECTED_QUEUE_H

#include "queue.hpp"
#include <atomic>
#include <cstdint>

namespace lockfree {

template <typename T>
class ABAProtectedQueue {
public:
    ABAProtectedQueue() : queue_(), version_counter_(0) {}
    
    void push(T value) {
        uintptr_t version = version_counter_.fetch_add(1, std::memory_order_relaxed);
        queue_.push({std::move(value), version});
    }
    
    bool pop(T& value) {
        std::pair<T, uintptr_t> pair;
        if (queue_.pop(pair)) {
            value = std::move(pair.first);
            return true;
        }
        return false;
    }
    
    bool empty() const { return queue_.empty(); }
    size_t size() const { return queue_.size(); }

private:
    Queue<std::pair<T, uintptr_t>> queue_;
    std::atomic<uintptr_t> version_counter_;
};

} // namespace lockfree

#endif // LOCKFREE_ABA_PROTECTED_QUEUE_H