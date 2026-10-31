#ifndef JOB_DATABASE_H
#define JOB_DATABASE_H

#include <stddef.h>
#include <stdint.h>

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

#ifdef __cplusplus
}
#endif

#endif
