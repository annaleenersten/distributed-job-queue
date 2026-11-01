#ifndef JOB_DATABASE_H
#define JOB_DATABASE_H

#include <stddef.h>
#include <stdint.h>

#define JOB_DATABASE_ERROR (-1)
#define JOB_DATABASE_NO_JOB 0
#define JOB_DATABASE_JOB_CLAIMED 1
#define JOB_DATABASE_LEASE_LOST 0
#define JOB_DATABASE_LEASE_RENEWED 1

typedef struct {
    int64_t id;
    char *command;
    int attempts;
} DatabaseJob;

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Inserts a queued job into PostgreSQL and writes its generated ID to job_id.
 * The connection string uses libpq conninfo syntax, for example:
 * "host=localhost port=5432 dbname=job_queue user=job_queue password=..."
 *
 * Returns 1 on success and 0 on failure. If error_message is non-NULL, a
 * human-readable error is copied into it (up to error_message_size bytes).
 * This function creates and closes its own connection, so callers may use it
 * from different HTTP threads without sharing a PGconn.
 */
int job_database_submit(
    const char *connection_string,
    const char *command,
    int64_t *job_id,
    char *error_message,
    size_t error_message_size
);

/*
 * Claims the oldest queued job, or an expired job that has retry attempts
 * remaining. Returns JOB_DATABASE_JOB_CLAIMED, JOB_DATABASE_NO_JOB, or
 * JOB_DATABASE_ERROR.
 */
int job_database_claim(
    const char *connection_string,
    const char *worker_id,
    int lease_seconds,
    DatabaseJob *job,
    char *error_message,
    size_t error_message_size
);

/*
 * Renews a running job's lease. Returns JOB_DATABASE_LEASE_RENEWED,
 * JOB_DATABASE_LEASE_LOST, or JOB_DATABASE_ERROR.
 */
int job_database_renew_lease(
    const char *connection_string,
    int64_t job_id,
    const char *worker_id,
    int lease_seconds,
    char *error_message,
    size_t error_message_size
);

/*
 * Marks a job completed when exit_code is zero, otherwise failed. Returns 1
 * when updated, 0 when this worker no longer owns the job, and -1 on error.
 */
int job_database_finish(
    const char *connection_string,
    int64_t job_id,
    const char *worker_id,
    int exit_code,
    const char *error_text,
    char *error_message,
    size_t error_message_size
);

void job_database_job_clear(DatabaseJob *job);

#ifdef __cplusplus
}
#endif

#endif
