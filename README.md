# Distributed Job Queue

A job queue system with an HTTP server that persists submitted jobs in PostgreSQL, plus a separate C in-memory queue and worker pool for local processing.

The project combines a thread-safe C job queue with a C++ HTTP server (https://github.com/annaleenersten/cpp-concurrent-server) for submitting and tracking jobs.

## Features

* FIFO in-memory queue and worker pool for local queue processing
* PostgreSQL persistence for submitted jobs
* In-memory job store (separate from PostgreSQL job records)
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
    +---- POST /jobs → PostgreSQL
    |
    +---- Cache routes
```

The in-memory queue library and worker pool are still available for local queue tests and are not connected to the HTTP submission path:

```text
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

The HTTP thread pool handles incoming requests. The current `/jobs` route inserts jobs into PostgreSQL and returns the generated database ID. The in-memory workers do not claim or execute those database jobs yet.

Each in-memory worker waits for available work using a condition variable, removes jobs from the FIFO queue, and executes them in a separate child process. Shared queue and job state are protected by a mutex.

## Job Lifecycle

```text
QUEUED → RUNNING → COMPLETED
                  ↘ FAILED
```

This lifecycle currently applies to the in-memory queue. PostgreSQL submissions remain `QUEUED` until database-backed workers are added.

## Requirements

* CMake 3.16+
* C compiler with C17 support
* C++ compiler with C++17 support
* POSIX-compatible operating system
* pthreads
* PostgreSQL client development library (`libpq`)
* Docker Engine and Docker Compose (for the local PostgreSQL container)
* GoogleTest

The worker implementation uses POSIX APIs including `fork()`, `execl()`, and `waitpid()`, so the project is intended for Linux or another POSIX-compatible environment.

On Ubuntu or WSL, install the PostgreSQL client development library with:

```bash
sudo apt update
sudo apt install libpq-dev
```

## Build

```bash
cmake -S . -B build
cmake --build build
```

## Start the local database

The Compose file starts a local PostgreSQL database. Start it from the project root:

```bash
docker compose up -d
```

Apply the schema to a new database volume:

```bash
docker compose exec -T db psql -U job_queue -d job_queue < database/schema.sql
```

You only need to apply the schema when initializing a new database volume (or when applying a schema change). The named `postgres_data` volume keeps database data when the container is stopped and restarted.

Stop the database container when you are finished:

```bash
docker compose down
```

This preserves the database volume. To also delete all local database data, use `docker compose down -v`.

## Run

Start the server:

```bash
export DATABASE_URL="host=localhost port=5432 dbname=job_queue user=job_queue password=local_dev_password"
./build/server
```

The server listens on port `8080`.

Submit a job:

```bash
curl -X POST http://localhost:8080/jobs -d "echo hello"
```

The server stores the job in PostgreSQL and returns its database ID. Jobs remain `QUEUED`; database-backed workers that claim and execute them are not implemented yet.

Multiple jobs can be submitted concurrently:

```bash
curl -X POST http://localhost:8080/jobs -d "sleep 5" &
curl -X POST http://localhost:8080/jobs -d "sleep 5" &
curl -X POST http://localhost:8080/jobs -d "sleep 5" &
```

These requests create persistent database jobs. They are not executed until database-backed workers are implemented.

## Testing

Run the complete test suite with:

```bash
cd build
ctest --output-on-failure
```

Tests cover the job queue, job store, HTTP server, routing, thread pool, cache functionality, and HTTP job submission.
