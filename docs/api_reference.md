# API Reference (Linux Kernel Style)

## Lockfree Queue Implementation

### `template<typename T> class Queue`
Thread-safe lock-free queue following Linux kernel coding standards.

Key Style Features:
- 80-character line limits
- Kernel-style brace formatting  
- 8-space tab indentation
- Minimal comments (self-documenting)
- Error handling via return values

#### Public Interface
| Method | Description |
|--------|-------------|
| `Queue()` | Construct empty queue |
| `~Queue()` | Cleanup resources |
| `push(T val)` | Add to queue (returns void) |
| `pop(T& val)` | Remove from queue (returns bool) |
| `empty()` | Check if empty |
| `size()` | Get element count |

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
