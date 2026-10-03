cmake_minimum_required(VERSION 3.16)

project(DistributedJobQueue
    VERSION 1.0
    LANGUAGES C CXX
)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


# -------------------------
# Job Queue Library
# -------------------------

add_library(job_queue
    queue/src/Job.c
    queue/src/Queue.c
    queue/src/Worker.c
    queue/src/JobStore.c
    queue/src/JobQueue.c
)

target_include_directories(job_queue PUBLIC
    queue/include
)


# -------------------------
# HTTP Server
# -------------------------

add_executable(server
    server/src/main.cpp
    server/src/Server.cpp
    server/src/Router.cpp
    server/src/HttpRequest.cpp
    server/src/HttpResponse.cpp
    server/src/ThreadPool.cpp
    server/src/Cache.cpp
)

target_include_directories(server PRIVATE
    server/include
)

target_link_libraries(server PRIVATE
    job_queue
)


# -------------------------
# GoogleTest
# -------------------------

include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
)

FetchContent_MakeAvailable(googletest)

enable_testing()


# -------------------------
# Server Tests
# -------------------------

add_executable(server_tests
    tests/Test_Server/CacheTest.cpp
    tests/Test_Server/HttpRequestTest.cpp
    tests/Test_Server/HttpResponseTest.cpp
    tests/Test_Server/RouterTest.cpp
    tests/Test_Server/ThreadPoolTest.cpp
    tests/Test_Server/ServerIntegrationTest.cpp

    server/src/Cache.cpp
    server/src/HttpRequest.cpp
    server/src/HttpResponse.cpp
    server/src/Router.cpp
    server/src/ThreadPool.cpp
)

target_include_directories(server_tests PRIVATE
    server/include
    queue/include
)

target_link_libraries(server_tests PRIVATE
    GTest::gtest_main
    job_queue
)

include(GoogleTest)
gtest_discover_tests(server_tests)


# -------------------------
# Queue Tests
# -------------------------

add_executable(queue_tests
    tests/Test_Queue/QueueTest.c

    queue/src/JobQueue.c
    queue/src/Job.c
    queue/src/Queue.c
    queue/src/JobStore.c
    queue/src/Worker.c
)

target_include_directories(queue_tests PRIVATE
    queue/include
)

add_test(
    NAME QueueTests
    COMMAND queue_tests
)