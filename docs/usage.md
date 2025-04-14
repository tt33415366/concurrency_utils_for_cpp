# Usage Examples

## Lock-free Queue
```cpp
lockfree::Queue<int> queue;

// Producer thread
queue.push(42);

// Consumer thread
int value;
if (queue.pop(value)) {
    // Process value
}
```

## Thread Pool
```cpp
lockfree::ThreadPool pool(4);

// Submit tasks
auto future = pool.submit([]{
    return some_computation();
});

// Get result
auto result = future.get();

// Shutdown when done
pool.shutdown();
```

## Best Practices
1. Check queue empty() before popping to avoid unnecessary waits
2. Use futures for task results when using thread pool
3. Always call shutdown() before destroying thread pool
4. For batch processing, reuse thread pool instances
5. Size queue appropriately for your workload

## Error Handling
- Queue pop() returns false if empty
- Thread pool submit() throws if pool is shutdown
- Task exceptions are captured in futures
