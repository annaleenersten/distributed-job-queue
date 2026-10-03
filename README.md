# Distributed Job Queue

A job queue system built in C that combines a FIFO job queue and process-based workers with a concurrent HTTP server.

The C++ HTTP server in this project was originally developed as a standalone project (https://github.com/annaleenersten/cpp-concurrent-server) and has been incorporated here as the server component of the distributed job queue system. This repository extends it with the C-based job queue and worker system.

## Architecture

```text
HTTP Client
     |
     v
C++ HTTP Server
     |
     v
Job Queue
     |
     v
Worker
     |
     v
Job Execution
```

### Components

* **Server** — C++ HTTP server using TCP sockets, HTTP request parsing, routing, a thread pool, and a thread-safe cache.
* **Queue** — C job queue with FIFO scheduling, job storage, worker processing, and job status tracking.
* **Workers** — Execute jobs using `fork()`, `exec()`, and `waitpid()` and track whether jobs complete or fail.

## Project Structure

```text
distributed-job-queue/
├── CMakeLists.txt
├── README.md
│
├── queue/
│   ├── include/
│   │   ├── Job.h
│   │   ├── Queue.h
│   │   ├── Worker.h
│   │   ├── JobStore.h
│   │   └── JobQueue.h
│   └── src/
│       ├── Job.c
│       ├── Queue.c
│       ├── Worker.c
│       ├── JobStore.c
│       └── JobQueue.c
│
├── server/
│   ├── include/
│   │   ├── Server.h
│   │   ├── Router.h
│   │   ├── HttpRequest.h
│   │   ├── HttpResponse.h
│   │   ├── ThreadPool.h
│   │   └── Cache.h
│   └── src/
│       ├── main.cpp
│       ├── Server.cpp
│       ├── Router.cpp
│       ├── HttpRequest.cpp
│       ├── HttpResponse.cpp
│       ├── ThreadPool.cpp
│       └── Cache.cpp
│
└── tests/
    ├── Test_Queue/
    │   └── QueueTest.c
    └── Test_Server/
        ├── CacheTest.cpp
        ├── HttpRequestTest.cpp
        ├── HttpResponseTest.cpp
        ├── RouterTest.cpp
        ├── ThreadPoolTest.cpp
        └── ServerIntegrationTest.cpp
```

## Requirements

* CMake 3.16 or newer
* GCC/G++
* C17
* C++17
* Linux or WSL

GoogleTest is downloaded automatically by CMake when configuring the project.

## Build

From the project root:

```bash
cmake -S . -B build
cmake --build build
```

This creates the main server and test executables in the `build/` directory:

```text
build/
├── server
├── server_tests
└── queue_tests
```

## Run the Server

Start the server with:

```bash
./build/server
```

The server listens on port `8080`.

You should see:

```text
Server listening on port 8080...
```

Keep the server running while using the integration tests.

Press `Ctrl+C` to stop the server.

## Run Queue Tests

The queue tests do not require the HTTP server to be running.

Run:

```bash
./build/queue_tests
```

A successful run should print:

```text
All queue tests passed!
```

## Run Server Tests

The server tests include unit tests and integration tests.

### Unit Tests

The unit tests cover:

* Cache
* HTTP request parsing
* HTTP response generation
* Routing
* Thread pool

These tests can run without starting the server.

### Integration Tests

`ServerIntegrationTest.cpp` connects to the running HTTP server on port `8080`.

**The server must already be running before running the integration tests.**

In one terminal:

```bash
./build/server
```

Then, in a second terminal:

```bash
cd build
ctest --output-on-failure
```

If the server is not running, the integration tests that connect to port `8080` will fail.

## Run All Tests

Start the server first:

```bash
./build/server
```

Then open another terminal and run:

```bash
cd build
ctest --output-on-failure
```

This runs the C queue tests and the C++ server tests through CTest.

## Clean Build

To remove the existing build directory and configure the project from scratch:

```bash
rm -rf build
cmake -S . -B build
cmake --build build
```

## Current Features

### Job Queue

* FIFO job queue
* Job creation and unique IDs
* Job status tracking
* In-memory job storage
* Job submission and retrieval
* Worker-based job processing
* Success and failure tracking

### Workers

Workers execute jobs as separate processes using:

```text
fork()
exec()
waitpid()
```

A job is marked as:

```text
QUEUED
   |
   v
RUNNING
   |
   +----> COMPLETED
   |
   +----> FAILED
```

### HTTP Server

* TCP socket creation and configuration
* HTTP/1.1 request parsing
* HTTP response generation
* Request routing
* Concurrent client handling
* Thread pool
* Thread-safe cache
* Cache statistics
* Integration testing

## future additions include:

* Multiple worker processes
* Job retries
* Job status endpoints
* Queue monitoring
* Performance benchmarking
* Persistent job storage
* Additional concurrency testing