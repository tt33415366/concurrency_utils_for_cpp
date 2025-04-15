#ifndef LOCKFREE_QUEUE_IPP
#define LOCKFREE_QUEUE_IPP

namespace lockfree {

template <typename T>
thread_local typename Queue<T>::HazardPointer Queue<T>::hp_;

template <typename T>
Queue<T>::Queue() : 
    head_(new Node(T())),
    tail_(head_.load()),
    size_(0) {
    hp_.ptr.store(nullptr, std::memory_order_relaxed);
}

template <typename T>
void Queue<T>::push(T value) {
    Node* new_node = new Node(std::move(value));
    Node* old_tail = tail_.exchange(new_node, std::memory_order_acq_rel);
    std::atomic_thread_fence(std::memory_order_release);
    old_tail->next.store(new_node, std::memory_order_release);
    size_.fetch_add(1, std::memory_order_relaxed);
}

template <typename T>
bool Queue<T>::pop(T& value) {
    Node* old_head = nullptr;
    Node* next_node = nullptr;
    
    while (true) {
        old_head = head_.load(std::memory_order_acquire);
        if (!old_head) {
            hp_.ptr.store(nullptr, std::memory_order_release);
            return false;
        }
        
        hp_.ptr.store(old_head, std::memory_order_release);
        if (head_.load(std::memory_order_acquire) != old_head) {
            continue;
        }
        
        next_node = old_head->next.load(std::memory_order_acquire);
        if (!next_node) {
            hp_.ptr.store(nullptr, std::memory_order_release);
            return false;
        }
        
        if (head_.compare_exchange_strong(
            old_head,
            next_node,
            std::memory_order_release,
            std::memory_order_relaxed)) {
            break;
        }
    }

    value = std::move(next_node->data);
    size_.fetch_sub(1, std::memory_order_relaxed);
    
    // Safe to retire old head now
    retire_node(old_head);
    hp_.ptr.store(nullptr, std::memory_order_release);
    return true;
}

template <typename T>
void Queue<T>::retire_node(Node* node) {
    if (!node) return;
    
    std::lock_guard<std::mutex> lock(retired_mutex_);
    retired_nodes_.push_back(node);
    if (retired_nodes_.size() >= 100) {
        scan();
    }
}

template <typename T>
void Queue<T>::scan() {
    std::vector<Node*> survivors;
    Node* protected_ptr = hp_.ptr.load(std::memory_order_acquire);
    
    for (auto node : retired_nodes_) {
        if (node == protected_ptr) {
            survivors.push_back(node);
        } else {
            delete node;
        }
    }
    retired_nodes_ = std::move(survivors);
}

template <typename T>
Queue<T>::~Queue() {
    for (auto node : retired_nodes_) {
        delete node;
    }
    head_.store(nullptr);
    tail_.store(nullptr);
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

#endif // LOCKFREE_QUEUE_IPP
