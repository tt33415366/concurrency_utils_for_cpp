# API Reference

## Lockfree Queue

### `template<typename T> class Queue`
Thread-safe lock-free queue implementation.

#### Public Members
| Method | Description |
|--------|-------------|
| `Queue()` | Constructs an empty queue |
| `~Queue()` | Destructor |
| `void push(T value)` | Pushes value to queue |
| `bool pop(T& value)` | Pops value from queue |
| `bool empty() const` | Checks if queue is empty |
| `size_t size() const` | Returns number of elements |

## Thread Pool

### `class ThreadPool`
Thread pool using lock-free queue for task management.

#### Public Members
| Method | Description |
|--------|-------------|
| `explicit ThreadPool(size_t threads)` | Constructs thread pool |
| `~ThreadPool()` | Destructor (calls shutdown()) |
| `template<typename F> auto submit(F&& f)` | Submits task to pool |
| `void wait()` | Waits for tasks to complete |
| `void shutdown()` | Stops accepting new tasks |
