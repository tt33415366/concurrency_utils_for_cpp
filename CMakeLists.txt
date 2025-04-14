cmake_minimum_required(VERSION 3.14)
project(concurrency_in_cpp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Release build flags
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")

# Google Test setup
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/release-1.8.1.tar.gz
)
FetchContent_MakeAvailable(googletest)

# Header-only library target
add_library(lockfree_queue INTERFACE)

target_sources(lockfree_queue INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include/lockfree/queue.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/lockfree/queue.ipp
)

target_include_directories(lockfree_queue INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Google Test setup
include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}/_deps/googletest-src/googletest/include
    ${CMAKE_CURRENT_BINARY_DIR}/_deps/googletest-src/googletest
)

# Test executables
add_executable(test_queue
    tests/test_queue.cpp
)

add_executable(test_thread_pool
    tests/test_thread_pool.cpp
)

# Link test executables
target_link_libraries(test_queue
    PRIVATE
    lockfree_queue
    gtest
    gtest_main
    pthread
)

target_link_libraries(test_thread_pool
    PRIVATE
    lockfree_queue
    gtest
    gtest_main
    pthread
)

# Copy Google Test headers to build directory
file(COPY ${CMAKE_CURRENT_BINARY_DIR}/_deps/googletest-src/googletest/include/gtest
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/include)

# Google Benchmark setup
include(FetchContent)
FetchContent_Declare(
  benchmark
  URL https://github.com/google/benchmark/archive/v1.7.0.tar.gz
)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Disable benchmark tests")
FetchContent_MakeAvailable(benchmark)

# Benchmark executables
add_executable(queue_bench
    benchmarks/queue_bench.cpp
)

add_executable(queue_concurrent_bench
    benchmarks/queue_concurrent_bench.cpp
)

add_executable(thread_pool_bench
    benchmarks/thread_pool_bench.cpp
)

add_executable(simple_thread_pool_bench
    benchmarks/simple_thread_pool_bench.cpp
)

target_link_libraries(queue_bench
    PRIVATE
    lockfree_queue
    benchmark::benchmark
    pthread
)

target_link_libraries(queue_concurrent_bench
    PRIVATE
    lockfree_queue
    benchmark::benchmark
    pthread
)

target_link_libraries(thread_pool_bench
    PRIVATE
    lockfree_queue
    benchmark::benchmark
    pthread
)

target_link_libraries(simple_thread_pool_bench
    PRIVATE
    lockfree_queue
    pthread
)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

enable_testing()
add_test(NAME test_queue COMMAND test_queue)
add_test(NAME test_thread_pool COMMAND test_thread_pool)
