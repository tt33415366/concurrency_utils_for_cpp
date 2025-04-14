#ifndef LOCKFREE_QUEUE_H
#define LOCKFREE_QUEUE_H

#include <atomic>
#include <memory>

/* Linux-style namespace */
namespace lfq {

template <typename T>
class Queue {
public:
    Queue();
    ~Queue();

    // Disable copying
    Queue(const Queue&) = delete;
    Queue& operator=(const Queue&) = delete;

    void push(T value);
    bool pop(T& value);
    bool empty() const;
    size_t size() const;

private:
    struct Node {
        T data;
        std::atomic<Node*> next;

        Node(T value) : data(std::move(value)), next(nullptr) {}
    };

    std::atomic<Node*> head_;
    std::atomic<Node*> tail_;
    std::atomic<size_t> size_;
};

} // namespace lfq

#include "queue.ipp"

#endif /* LOCKFREE_QUEUE_H */
