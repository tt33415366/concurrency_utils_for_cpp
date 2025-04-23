#include <iostream>
#include <atomic>
#include <memory>
#include "../../include/lockfree/queue.hpp"

int main() {
    lockfree::Queue<int> q;
    
    // Test basic operations
    q.push(42);
    int val;
    if (q.pop(val)) {
        std::cout << "Success! Popped value: " << val << std::endl;
        return 0;
    }
    
    std::cerr << "Error: Failed to pop value" << std::endl;
    return 1;
}