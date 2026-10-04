# Distributed Job Queue

A job queue system built in C that accepts jobs through an HTTP server, queues them for processing, and executes multiple jobs concurrently using a pool of background workers.

The project combines a thread-safe C job queue with a C++ HTTP server (https://github.com/annaleenersten/cpp-concurrent-server) for submitting and tracking jobs.

## Features

* FIFO job queue for managing submitted work
* In-memory job store with job status tracking
* Three-worker processing pool for concurrent job execution
* Thread-safe queue and job store using POSIX mutexes
* Condition variables for worker synchronization
* Process-based job execution using `fork()`, `execl()`, and `waitpid()`
* Job lifecycle tracking: `QUEUED`, `RUNNING`, `COMPLETED`, and `FAILED`
* Graceful shutdown
* HTTP interface for submitting jobs
* Unit and integration testing

## Architecture

```text
HTTP Client
    |
    v
C++ HTTP Server
    |
    v
HTTP Thread Pool
    |
    v
C Job Queue
    |
    +---- Job Store
    |
    +---- FIFO Queue
    |
    v
Worker Pool
    |
    +---- Worker 1
    +---- Worker 2
    +---- Worker 3
    |
    v
fork() → execute command
```

The HTTP thread pool handles incoming requests, while the worker pool processes queued jobs independently.

Each worker waits for available work using a condition variable, removes jobs from the FIFO queue, and executes them in a separate child process. Shared queue and job state are protected by a mutex.

## Job Lifecycle

```text
QUEUED → RUNNING → COMPLETED
                  ↘ FAILED
```

## Requirements

* CMake 3.16+
* C compiler with C17 support
* C++ compiler with C++17 support
* POSIX-compatible operating system
* pthreads
* GoogleTest

The worker implementation uses POSIX APIs including `fork()`, `execl()`, and `waitpid()`, so the project is intended for Linux or another POSIX-compatible environment.

## Build

```bash
cmake -S . -B build
cmake --build build
```

## Run

Start the server:

```bash
./build/server
```

The server listens on port `8080`.

Submit a job:

```bash
curl -X POST http://localhost:8080/jobs -d "echo hello"
```

The server returns a job ID and adds the job to the queue for background processing.

Multiple jobs can be submitted concurrently:

```bash
curl -X POST http://localhost:8080/jobs -d "sleep 5" &
curl -X POST http://localhost:8080/jobs -d "sleep 5" &
curl -X POST http://localhost:8080/jobs -d "sleep 5" &
```

With three workers, the jobs can execute concurrently.

## Testing

Run the complete test suite with:

```bash
cd build
ctest --output-on-failure
```

Tests cover the job queue, job store, HTTP server, routing, thread pool, cache functionality, and HTTP job submission.
