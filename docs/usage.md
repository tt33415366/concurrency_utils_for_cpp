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

## Thread Pool
```cpp
lfq::ThreadPool pool(4);  // Note: namespace changed to lfq

// Submit tasks
auto future = pool.submit([]{
    return some_computation();
});

// Get result
auto result = future.get();

// Shutdown when done
pool.shutdown();
```

## Best Practices (Linux Style)
1. Check empty() before pop() - avoids waits
2. Use futures for task results
3. Call shutdown() before destroy
4. Reuse pool instances
5. Size queue for workload
6. Style compliance:
   - 80-char line limits
   - Explicit error checks
   - Kernel-style returns

## Error Handling (Linux Style)
- Queue pop(): returns false if empty
- Thread pool submit(): returns error if shutdown
- Tasks: exceptions in futures
- Style notes:
  - Prefer return codes over exceptions
  - Document all error cases
  - Check all returns
