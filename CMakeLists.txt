cmake_minimum_required(VERSION 3.14)
project(concurrency_in_cpp LANGUAGES CXX)

# Use C++11 standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Release build flags
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")

# Benchmark setup
option(ENABLE_BENCHMARKS "Build benchmark tests" ON)

if(ENABLE_BENCHMARKS)
    # Disable benchmark's own tests
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
    add_subdirectory(${CMAKE_SOURCE_DIR}/third_party/benchmark)
    
    # Queue benchmarks
    add_executable(queue_bench benchmarks/queue_bench.cpp)
    target_link_libraries(queue_bench PRIVATE lockfree_queue benchmark pthread)

    # Thread pool benchmarks 
    add_executable(thread_pool_bench benchmarks/thread_pool_bench.cpp)
    target_link_libraries(thread_pool_bench PRIVATE lockfree_queue benchmark pthread)
endif()

# Google Test setup
if(NOT TARGET gtest)
    message(STATUS "Using local Google Test from third_party")
    add_subdirectory(${CMAKE_SOURCE_DIR}/third_party/googletest)
    include(GoogleTest)
endif()

# Set up include directories
include_directories(
    SYSTEM ${CMAKE_SOURCE_DIR}/third_party/googletest/googletest/include
    SYSTEM ${CMAKE_SOURCE_DIR}/third_party/googletest/googlemock/include
    ${CMAKE_SOURCE_DIR}/include
)

# Enable testing
enable_testing()

# Header-only library target
add_library(lockfree_queue INTERFACE)

target_sources(lockfree_queue INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include/lockfree/queue.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/lockfree/queue.ipp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/lockfree/thread_pool.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/lockfree/thread_pool.ipp
)

target_include_directories(lockfree_queue INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Google Test setup
include_directories(
    ${CMAKE_SOURCE_DIR}/third_party/googletest/googletest/include
    ${CMAKE_SOURCE_DIR}/third_party/googletest/googlemock/include
)

# Test executables
add_executable(test_queue
    tests/test_queue.cpp
)

add_executable(test_thread_pool
    tests/test_thread_pool.cpp
)

add_executable(test_aba_protected_queue
    tests/test_aba_protected_queue.cpp
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

target_link_libraries(test_aba_protected_queue
    PRIVATE
    lockfree_queue
    gtest
    gtest_main
    pthread
)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Minimal test executable
add_executable(minimal_test
    tests/minimal/minimal_test.cpp
)

target_link_libraries(minimal_test
    PRIVATE
    gtest
    gtest_main
    pthread
)

enable_testing()
add_test(NAME test_queue COMMAND test_queue)
add_test(NAME test_thread_pool COMMAND test_thread_pool)
add_test(NAME test_aba_protected_queue COMMAND test_aba_protected_queue)
add_test(NAME minimal_test COMMAND minimal_test)
