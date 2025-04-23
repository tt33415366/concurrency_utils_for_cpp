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
    Node* old_head = head_.load(std::memory_order_acquire);
    if (!old_head) return false;  // Queue is in shutdown state
    
    Node* next_node;
    do {
        next_node = old_head->next.load(std::memory_order_acquire);
        if (!next_node) return false;
        
        // Validate pointers before access
        if (!old_head || !next_node) {
            return false;
        }
    } while (!head_.compare_exchange_weak(
        old_head, next_node,
        std::memory_order_acq_rel,
        std::memory_order_acquire));

    try {
        value = std::move(next_node->data);
        delete old_head;
        size_.fetch_sub(1, std::memory_order_relaxed);
        return true;
    } catch (...) {
        // Ensure we don't leak memory if move fails
        delete old_head;
        throw;
    }
}

template <typename T>
bool Queue<T>::empty() const {
    return size_.load(std::memory_order_acquire) == 0;
}

template <typename T>
size_t Queue<T>::size() const {
    return size_.load(std::memory_order_acquire);
}

template <typename T>
std::atomic<size_t> Queue<T>::Node::active_nodes{0};

template <typename T>
void Queue<T>::clear() {
    Node* old_head = head_.exchange(nullptr, std::memory_order_seq_cst);
    tail_.store(nullptr, std::memory_order_seq_cst);
    size_.store(0, std::memory_order_relaxed);
    
    Node* current = old_head;
    while (current) {
        Node* next = current->next.load(std::memory_order_relaxed);
        delete current;
        current = next;
    }
}

template <typename T>
size_t Queue<T>::get_active_nodes() {
    return Node::active_nodes.load(std::memory_order_relaxed);
}

template <typename T>
void Queue<T>::force_release_nodes() {
    Node::active_nodes.store(0, std::memory_order_relaxed);
}

} // namespace lockfree

#endif /* LOCKFREE_QUEUE_IPP */
