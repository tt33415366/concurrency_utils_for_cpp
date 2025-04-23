#ifndef LOCKFREE_QUEUE_H
#define LOCKFREE_QUEUE_H

#include <atomic>
#include <memory>

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
    
    void clear();
    static size_t get_active_nodes();
    static void force_release_nodes();

private:
    struct Node {
        T data;
        std::atomic<Node*> next;
        static std::atomic<size_t> active_nodes;

        Node(T value) : data(std::move(value)), next(nullptr) {
            active_nodes.fetch_add(1, std::memory_order_relaxed);
        }
        ~Node() {
            active_nodes.fetch_sub(1, std::memory_order_relaxed);
        }
    };

    std::atomic<Node*> head_;
    std::atomic<Node*> tail_;
    std::atomic<size_t> size_;
};

} // namespace lockfree

#include "queue.ipp"

#endif /* LOCKFREE_QUEUE_H */
