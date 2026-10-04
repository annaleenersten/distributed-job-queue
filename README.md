# Distributed Job Queue

A job queue system built in C that accepts jobs through an HTTP server, queues them for processing, and executes multiple jobs concurrently using a pool of background workers.

The project combines a thread-safe C job queue with a concurrent C++ HTTP server. The HTTP server was originally developed as a standalone project, (https://github.com/annaleenersten/cpp-concurrent-server), and is incorporated here as the interface for submitting jobs. This project extends the server with job scheduling, worker processes, job status tracking, and concurrent job execution.

## Features

* **FIFO job queue** for managing submitted work
* **In-memory job store** for tracking job IDs, commands, and execution status
* **Three-worker pool** for concurrent job processing
* **Thread-safe queue and job store** protected by a mutex
* **Condition variable synchronization** so workers sleep while the queue is empty
* **Process-based job execution** using `fork()`, `execl()`, and `waitpid()`
* **Job lifecycle tracking** with `QUEUED`, `RUNNING`, `COMPLETED`, and `FAILED` states
* **Graceful shutdown** that allows queued jobs to finish before workers exit
* **Concurrent HTTP server** with a reusable thread pool for handling client requests
* **Unit and integration tests** for the queue, server, and HTTP job submission

## Architecture

The system separates **receiving work** from **processing work**.

```text
                         HTTP Clients
                              |
                              v
                    +-------------------+
                    |   C++ HTTP Server |
                    +-------------------+
                              |
                    HTTP Thread Pool
                              |
                              v
                    +-------------------+
                    |  Job Queue System |
                    +-------------------+
                       |      |      |
                       |      |      |
                  JobStore   FIFO   Synchronization
                             Queue   (Mutex + CV)
                               |
                    +----------+----------+
                    |          |          |
                    v          v          v
                 Worker 1   Worker 2   Worker 3
                    |          |          |
                    +----------+----------+
                               |
                              fork()
                               |
                               v
                       Child Process
                               |
                         Execute Command
                               |
                               v
                      COMPLETED / FAILED
```

The **HTTP thread pool** handles incoming client requests, while the **job worker pool** processes submitted jobs. The two pools have different responsibilities and operate independently.

When a job is submitted:

1. The HTTP server receives the request.
2. The request body is used as the job command.
3. The job is stored in the `JobStore` with a unique ID and `QUEUED` status.
4. A copy of the job is added to the FIFO queue.
5. A worker is notified through the condition variable.
6. An available worker removes the job from the queue and marks it `RUNNING`.
7. The worker executes the command in a separate child process.
8. The job is marked `COMPLETED` or `FAILED` after execution.

Because there are three workers, multiple queued jobs can be processed at the same time.

## Components

### Job

Represents a unit of work.

Each job contains:

* Unique job ID
* Command to execute
* Current execution status

### Queue

A linked-list FIFO queue used to schedule jobs waiting for a worker.

### JobStore

An in-memory store containing submitted jobs and their current status.

The queue determines **what should be processed next**, while the job store tracks **what has happened to each job**.

### JobQueueSystem

Coordinates the queue, job store, mutex, condition variable, and worker processing.

## Job Lifecycle

```text
             Submit
                |
                v
             QUEUED
                |
                v
            RUNNING
             /     \
            v       v
      COMPLETED    FAILED
```

Workers update the job status as it moves through the execution lifecycle.

## Synchronization

The job queue system uses a `pthread_mutex_t` to protect shared queue and job-store state.

Workers follow this general pattern:

```text
Lock mutex
    |
    +-- Remove job from queue
    +-- Mark job RUNNING
    |
Unlock mutex
    |
Execute job
    |
Lock mutex
    |
    +-- Mark job COMPLETED or FAILED
    |
Unlock mutex
```

The mutex is only held while accessing shared state, so workers do not block each other while a job is actually executing.

A condition variable allows workers to wait efficiently when there are no jobs available instead of continuously polling the queue.

## Job Execution

Each job is executed in a separate child process.

```text
Worker
  |
  +-- fork()
       |
       +-- Child → execl() → execute command
       |
       +-- Parent → waitpid() → collect result
```

The current implementation executes commands through `/bin/sh`, making this project suitable as a local systems-programming prototype rather than a publicly exposed job execution service.

## Requirements

* CMake 3.16 or newer
* C compiler with C17 support
* C++ compiler with C++17 support
* POSIX-compatible operating system
* pthreads
* GoogleTest (downloaded automatically by CMake)

The worker implementation uses POSIX APIs including `fork()`, `execl()`, and `waitpid()`, so the project is intended for Linux or another POSIX-compatible environment.

## Build

From the project root:

```bash
cmake -S . -B build
cmake --build build
```

## Run the Server

From the project root:

```bash
./build/server
```

The server listens on port `8080`.

Keep the server running while running the HTTP integration tests.

## Submit a Job

Jobs are submitted using an HTTP `POST` request to `/jobs`.

For example:

```bash
curl -X POST http://localhost:8080/jobs -d "echo hello"
```

The server creates a job, adds it to the queue, and returns the assigned job ID.

The worker pool then processes the job in the background.

Example output:

```text
Worker processing job 1: echo hello
hello
Job 1 completed.
```

### Concurrent Jobs

Multiple long-running jobs can be submitted to demonstrate concurrent processing:

```bash
curl -X POST http://localhost:8080/jobs -d "sleep 5" &
curl -X POST http://localhost:8080/jobs -d "sleep 5" &
curl -X POST http://localhost:8080/jobs -d "sleep 5" &
```

With three workers, the jobs can execute concurrently rather than waiting for each previous job to finish.

## Run Tests

### Queue Tests

From the project root:

```bash
./build/queue_tests
```

### Server Tests

```bash
./build/server_tests
```

Server tests include unit tests for:

* HTTP request parsing
* HTTP response generation
* Routing
* Thread pool behavior
* Cache functionality

The server integration tests verify HTTP communication with the running server.

### All Tests

```bash
cd build
ctest --output-on-failure
```

## Clean Build

To remove the existing build and create a fresh build:

```bash
rm -rf build
cmake -S . -B build
cmake --build build
```

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
│   │
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
│   │
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
    │
    └── Test_Server/
        ├── CacheTest.cpp
        ├── HttpRequestTest.cpp
        ├── HttpResponseTest.cpp
        ├── RouterTest.cpp
        ├── ThreadPoolTest.cpp
        └── ServerIntegrationTest.cpp
```
