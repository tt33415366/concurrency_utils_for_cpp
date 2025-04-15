#ifndef LOCKFREE_QUEUE_H
#define LOCKFREE_QUEUE_H

#include <atomic>
#include <memory>
#include <vector>
#include <mutex>

namespace lockfree {

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

    // Hazard pointer record
    struct HazardPointer {
        std::atomic<Node*> ptr;
        HazardPointer* next;
    };

    static thread_local HazardPointer hp_;

    std::atomic<Node*> head_;
    std::atomic<Node*> tail_;
    std::atomic<size_t> size_;
    std::vector<Node*> retired_nodes_;
    std::mutex retired_mutex_;

    void retire_node(Node* node);
    void scan();
};

} // namespace lockfree

#include "queue.ipp"

#endif /* LOCKFREE_QUEUE_H */
