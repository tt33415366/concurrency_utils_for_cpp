#ifndef LOCKFREE_QUEUE_IPP
#define LOCKFREE_QUEUE_IPP

namespace lfq {

template <typename T>
Queue<T>::Queue() : head_(new Node(T())), tail_(head_.load()), size_(0) {}

template <typename T>
Queue<T>::~Queue() {
    while (Node* old_head = head_.load()) {
        head_.store(old_head->next);
        delete old_head;
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
    Node* next_node = nullptr;
    
    while (old_head) {
        next_node = old_head->next.load(std::memory_order_acquire);
        
        if (!next_node) {
            return false;
        }

        // Try to move the head atomically
        if (head_.compare_exchange_weak(
                old_head, 
                next_node,
                std::memory_order_release,
                std::memory_order_acquire)) {
            value = std::move(next_node->data);
            size_.fetch_sub(1, std::memory_order_relaxed);
            
            // Safe to delete only if no other threads could be accessing it
            std::atomic_thread_fence(std::memory_order_acquire);
            delete old_head;
            return true;
        }
    }
    return false;
}

template <typename T>
bool Queue<T>::empty() const {
    return size_.load(std::memory_order_acquire) == 0;
}

template <typename T>
size_t Queue<T>::size() const {
    return size_.load(std::memory_order_acquire);
}

} // namespace lfq

#endif // LOCKFREE_QUEUE_IPP
