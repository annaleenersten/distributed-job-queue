# Distributed Job Queue

A job queue system with an HTTP server that persists submitted jobs in PostgreSQL and a worker pool that claims and executes jobs from the database.

The project combines a thread-safe C job queue ([job-queue](https://github.com/annaleenersten/job-queue)) with a C++ HTTP server ([cpp-concurrent-server](https://github.com/annaleenersten/cpp-concurrent-server)) for submitting and tracking jobs.

This project extends the original in-memory queue by moving job state into PostgreSQL, so multiple server instances on different machines can share one queue.

## Features

* Three database-backed workers that claim jobs concurrently
* PostgreSQL persistence for submitted jobs
* Atomic PostgreSQL job claims with `FOR UPDATE SKIP LOCKED`
* Worker leases, heartbeat renewal, and bounded retries after worker failure
* C in-memory queue retained for local queue tests
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

The server starts three workers that poll PostgreSQL and claim available jobs. Each claim sets a worker ID, increments the attempt count, and creates a 90-second lease. Workers renew leases while commands run, then record the exit code and final status. Expired jobs are retried up to `max_attempts`.

```text
PostgreSQL
    |
    +---- QUEUED jobs
    |
    +---- atomic claim + lease
    |
    v
Three worker threads
    |
    +---- Worker 1
    +---- Worker 2
    +---- Worker 3
    |
    v
fork() → execute command
```

The HTTP thread pool handles incoming requests. The `/jobs` route inserts jobs into PostgreSQL and returns the generated database ID. The C in-memory queue remains available for local queue tests but is not used by the HTTP server.

## Job Lifecycle

```text
QUEUED → RUNNING → COMPLETED
                  ↘ FAILED
```

Database-backed workers update each job through this lifecycle and record its exit code and error details.

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

For an existing `postgres_data` volume, PostgreSQL keeps the password set when that volume was first initialized; changing `POSTGRES_PASSWORD` in Compose does not change the database role's password. Make sure `DATABASE_URL` uses the password already configured in the database.

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
export DATABASE_URL="host=localhost port=5433 dbname=job_queue user=job_queue password=local_dev_password"
./build/server
```

The server listens on port `8080`.

Submit a job:

```bash
curl -X POST http://localhost:8080/jobs -d "echo hello"
```

The server stores the job in PostgreSQL and returns its database ID. A database-backed worker claims and executes it, then updates its status.

Multiple jobs can be submitted concurrently:

```bash
curl -X POST http://localhost:8080/jobs -d "sleep 5" &
curl -X POST http://localhost:8080/jobs -d "sleep 5" &
curl -X POST http://localhost:8080/jobs -d "sleep 5" &
```

These requests create persistent database jobs that the worker pool can execute concurrently.

## Testing

Run the complete test suite with:

```bash
cd build
ctest --output-on-failure
```

The `ServerIntegrationTest` cases connect to a live server on port `8080`. Start the server in another terminal first (with PostgreSQL configured if you also want to test job submissions). Tests cover the job queue, HTTP routing, thread pool, cache functionality, and HTTP server integration.
