# Usage Examples (Linux Kernel Style)


Kernel-style usage examples
- 80-character line limits
- Explicit error checking
- Minimal comments


## Lock-free Queue
```cpp
lockfree::Queue<int> queue;  // Updated namespace

// Single item operations
queue.push(42);  // Non-blocking
int value;
if (queue.pop(value)) {
    // Process value
}

// Bulk operations (3-5x faster)
std::vector<int> items = {1, 2, 3};
queue.push_bulk(items);  // Atomic multi-insert

std::vector<int> results;
if (queue.pop_bulk(results, 3)) {  // Atomic multi-remove
    // Process batch
}
```

## Thread Pool Configuration

Recommended settings based on workload:

```cpp
// CPU-bound tasks
lockfree::ThreadPool cpu_pool(std::thread::hardware_concurrency());

// IO-bound tasks  
lockfree::ThreadPool io_pool(std::thread::hardware_concurrency() * 2);

// Mixed workload
lockfree::ThreadPool mixed_pool(
    std::thread::hardware_concurrency() * 1.5
);

## Thread Pool Usage
```cpp
lockfree::ThreadPool pool;  // Defaults to hardware_concurrency

// Submit with backpressure
for (int i = 0; i < 1000; ++i) {
    auto future = pool.submit([i] {
        return process_item(i); 
    });
    futures.push_back(std::move(future));
}

// Monitor progress
std::cout << "Active: " << pool.active_tasks() 
          << " Pending: " << pool.pending_tasks() << "\n";

// Wait with timeout
if (pool.wait_for(std::chrono::seconds(5))) {
    std::cout << "All tasks completed\n";
} else {
    std::cout << "Timeout reached\n";
    pool.shutdown();  // Force shutdown
}

// Exception handling
try {
    auto result = futures[0].get();
} catch (const std::exception& e) {
    std::cerr << "Task failed: " << e.what() << "\n";
}
```

## Best Practices (Linux Style)

### Queue Usage
1. Prefer bulk operations for better throughput
2. Size queue appropriately for workload
3. Check empty() before pop() to avoid waits
4. Reuse queue instances to avoid allocation

### Thread Pool Usage  
1. Monitor active_tasks()/pending_tasks() for load
2. Use wait_for() with timeout for responsiveness
3. Reuse pool instances to avoid thread creation
4. Handle task exceptions via futures
5. Size pool based on workload characteristics

### General
1. Follow Linux kernel style:
   - 80-char line limits
   - Explicit error checks
   - Return code conventions
2. Profile before optimizing
3. Test edge cases:
   - Empty/full queue
   - Pool shutdown
   - Exception paths

## Error Handling (Linux Style)

### Queue Operations
- pop(): returns false if empty (no exceptions)
- push_bulk(): returns false if allocation fails
- pop_bulk(): returns false if empty
- Style: Consistent return code pattern

### Thread Pool
- submit(): throws runtime_error if shutdown
- wait_for(): returns false on timeout
- Tasks: Exceptions captured in futures
- Shutdown: No new tasks accepted

### General Principles
1. Document all error conditions
2. Check all return values
3. Prefer return codes over exceptions where possible
4. Test error paths:
   - Queue full/empty conditions
   - Pool shutdown scenarios
   - Allocation failures
5. Style compliance:
   - 80-char line limits
   - Explicit error checks
   - Kernel-style returns
