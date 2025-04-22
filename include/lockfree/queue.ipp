#ifndef LOCKFREE_QUEUE_IPP
#define LOCKFREE_QUEUE_IPP

#include <iostream>

namespace lockfree {

template <typename T>
Queue<T>::Queue() : 
    head_(new Node(T{})), 
    tail_(head_.load()),
    size_(0) {}

template <typename T>
Queue<T>::~Queue() {
    while (Node* node = head_.load()) {
        head_.store(node->next);
        delete node;
    }
}

template <typename T>
void Queue<T>::push(T value) {
    Node* new_node = new Node(std::move(value));
    Node* old_tail = tail_.exchange(new_node, std::memory_order_acq_rel);
    old_tail->next.store(new_node, std::memory_order_release);
    size_.fetch_add(1, std::memory_order_relaxed);
}

template <typename T>
bool Queue<T>::pop(T& value) {
    Node* old_head = head_.load(std::memory_order_relaxed);
    Node* next_node;

    do {
        next_node = old_head->next.load(std::memory_order_acquire);
        if (!next_node) return false;
    } while (!head_.compare_exchange_weak(
        old_head, next_node, 
        std::memory_order_acq_rel, 
        std::memory_order_relaxed));

    value = std::move(next_node->data);
    delete old_head;
    size_.fetch_sub(1, std::memory_order_relaxed);
    return true;
}

template <typename T>
bool Queue<T>::empty() const {
    return size_.load(std::memory_order_acquire) == 0;
}

template <typename T>
size_t Queue<T>::size() const {
    return size_.load(std::memory_order_acquire);
}

} // namespace lockfree

#endif /* LOCKFREE_QUEUE_IPP */
