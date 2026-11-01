#include "JobDatabase.h"

#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libpq-fe.h>

static void set_error(
    char *error_message,
    size_t error_message_size,
    const char *message
)
{
    if (error_message == NULL || error_message_size == 0) {
        return;
    }

    snprintf(error_message, error_message_size, "%s", message);
}

static PGconn *connect_database(
    const char *connection_string,
    char *error_message,
    size_t error_message_size
)
{
    if (connection_string == NULL || connection_string[0] == '\0') {
        set_error(error_message, error_message_size,
                  "Database connection string is empty");
        return NULL;
    }

    PGconn *connection = PQconnectdb(connection_string);
    if (connection == NULL) {
        set_error(error_message, error_message_size,
                  "Could not allocate a PostgreSQL connection");
        return NULL;
    }

    if (PQstatus(connection) != CONNECTION_OK) {
        set_error(error_message, error_message_size, PQerrorMessage(connection));
        PQfinish(connection);
        return NULL;
    }

    return connection;
}

static void set_result_error(
    PGconn *connection,
    PGresult *result,
    char *error_message,
    size_t error_message_size
)
{
    const char *message = PQerrorMessage(connection);
    if (result != NULL && PQresultErrorMessage(result)[0] != '\0') {
        message = PQresultErrorMessage(result);
    }
    set_error(error_message, error_message_size, message);
}

static int parse_int64(const char *text, int64_t *value)
{
    errno = 0;
    char *end = NULL;
    intmax_t parsed = strtoimax(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0' ||
        parsed < INT64_MIN || parsed > INT64_MAX) {
        return 0;
    }

    *value = (int64_t)parsed;
    return 1;
}

static int parse_int(const char *text, int *value)
{
    errno = 0;
    char *end = NULL;
    long parsed = strtol(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0' ||
        parsed < INT_MIN || parsed > INT_MAX) {
        return 0;
    }

    *value = (int)parsed;
    return 1;
}

int job_database_submit(
    const char *connection_string,
    const char *command,
    int64_t *job_id,
    char *error_message,
    size_t error_message_size
)
{
    if (error_message != NULL && error_message_size > 0) {
        error_message[0] = '\0';
    }

    if (command == NULL || command[0] == '\0') {
        set_error(error_message, error_message_size, "Job command is empty");
        return 0;
    }
    if (job_id == NULL) {
        set_error(error_message, error_message_size,
                  "Job ID output pointer is NULL");
        return 0;
    }

    size_t command_length = strlen(command);
    if (command_length > INT_MAX) {
        set_error(error_message, error_message_size, "Job command is too long");
        return 0;
    }

    PGconn *connection = connect_database(
        connection_string,
        error_message,
        error_message_size
    );
    if (connection == NULL) {
        return 0;
    }

    const char *parameter_values[] = { command };
    const int parameter_lengths[] = { (int)command_length };
    const int parameter_formats[] = { 0 };
    PGresult *result = PQexecParams(
        connection,
        "INSERT INTO jobs (command) VALUES ($1) RETURNING id",
        1,
        NULL,
        parameter_values,
        parameter_lengths,
        parameter_formats,
        0
    );

    int success = 0;
    if (result == NULL || PQresultStatus(result) != PGRES_TUPLES_OK ||
        PQntuples(result) != 1 || PQnfields(result) != 1) {
        set_result_error(connection, result, error_message, error_message_size);
    } else {
        success = parse_int64(PQgetvalue(result, 0, 0), job_id);
        if (!success) {
            set_error(error_message, error_message_size,
                      "PostgreSQL returned an invalid job ID");
        }
    }

    if (result != NULL) {
        PQclear(result);
    }
    PQfinish(connection);
    return success;
}

int job_database_claim(
    const char *connection_string,
    const char *worker_id,
    int lease_seconds,
    DatabaseJob *job,
    char *error_message,
    size_t error_message_size
)
{
    if (error_message != NULL && error_message_size > 0) {
        error_message[0] = '\0';
    }
    if (job != NULL) {
        job->id = 0;
        job->command = NULL;
        job->attempts = 0;
    }
    if (worker_id == NULL || worker_id[0] == '\0' ||
        lease_seconds <= 0 || job == NULL) {
        set_error(error_message, error_message_size,
                  "Invalid job claim arguments");
        return JOB_DATABASE_ERROR;
    }

    PGconn *connection = connect_database(
        connection_string,
        error_message,
        error_message_size
    );
    if (connection == NULL) {
        return JOB_DATABASE_ERROR;
    }

    char lease_text[32];
    snprintf(lease_text, sizeof(lease_text), "%d", lease_seconds);
    const char *parameter_values[] = { worker_id, lease_text };
    const char *query =
        "WITH exhausted_jobs AS ("
        "  UPDATE jobs"
        "  SET status = 'FAILED', finished_at = now(), worker_id = NULL,"
        "      lease_until = NULL,"
        "      error_text = COALESCE(error_text,"
        "          'Worker lease expired after maximum attempts')"
        "  WHERE status = 'RUNNING' AND lease_until <= now()"
        "    AND attempts >= max_attempts"
        "  RETURNING id"
        "), candidate AS ("
        "  SELECT id FROM jobs"
        "  WHERE attempts < max_attempts"
        "    AND (status = 'QUEUED'"
        "         OR (status = 'RUNNING' AND lease_until <= now()))"
        "  ORDER BY created_at, id"
        "  FOR UPDATE SKIP LOCKED"
        "  LIMIT 1"
        ")"
        "UPDATE jobs AS job"
        " SET status = 'RUNNING', worker_id = $1,"
        "     lease_until = now() + make_interval(secs => $2::double precision),"
        "     started_at = COALESCE(job.started_at, now()),"
        "     finished_at = NULL, attempts = job.attempts + 1,"
        "     error_text = CASE WHEN job.status = 'RUNNING'"
        "                       THEN 'Previous worker lease expired'"
        "                       ELSE NULL END"
        " FROM candidate"
        " WHERE job.id = candidate.id"
        " RETURNING job.id, job.command, job.attempts";
    PGresult *result = PQexecParams(
        connection,
        query,
        2,
        NULL,
        parameter_values,
        NULL,
        NULL,
        0
    );

    int outcome = JOB_DATABASE_ERROR;
    if (result == NULL || PQresultStatus(result) != PGRES_TUPLES_OK ||
        PQntuples(result) > 1 || PQnfields(result) != 3) {
        set_result_error(connection, result, error_message, error_message_size);
    } else if (PQntuples(result) == 0) {
        outcome = JOB_DATABASE_NO_JOB;
    } else {
        const char *command = PQgetvalue(result, 0, 1);
        size_t command_length = strlen(command);
        job->command = malloc(command_length + 1);
        if (job->command == NULL) {
            set_error(error_message, error_message_size,
                      "Could not allocate memory for the claimed job");
        } else if (!parse_int64(PQgetvalue(result, 0, 0), &job->id) ||
                   !parse_int(PQgetvalue(result, 0, 2), &job->attempts)) {
            set_error(error_message, error_message_size,
                      "PostgreSQL returned invalid job data");
            free(job->command);
            job->command = NULL;
        } else {
            memcpy(job->command, command, command_length + 1);
            outcome = JOB_DATABASE_JOB_CLAIMED;
        }
    }

    if (result != NULL) {
        PQclear(result);
    }
    PQfinish(connection);
    return outcome;
}

int job_database_renew_lease(
    const char *connection_string,
    int64_t job_id,
    const char *worker_id,
    int lease_seconds,
    char *error_message,
    size_t error_message_size
)
{
    if (error_message != NULL && error_message_size > 0) {
        error_message[0] = '\0';
    }
    if (worker_id == NULL || worker_id[0] == '\0' || lease_seconds <= 0) {
        set_error(error_message, error_message_size,
                  "Invalid job lease renewal arguments");
        return JOB_DATABASE_ERROR;
    }

    PGconn *connection = connect_database(
        connection_string,
        error_message,
        error_message_size
    );
    if (connection == NULL) {
        return JOB_DATABASE_ERROR;
    }

    char id_text[32];
    char lease_text[32];
    snprintf(id_text, sizeof(id_text), "%" PRId64, job_id);
    snprintf(lease_text, sizeof(lease_text), "%d", lease_seconds);
    const char *parameter_values[] = { id_text, worker_id, lease_text };
    PGresult *result = PQexecParams(
        connection,
        "UPDATE jobs"
        " SET lease_until = now() + make_interval(secs => $3::double precision)"
        " WHERE id = $1 AND worker_id = $2 AND status = 'RUNNING'"
        "   AND lease_until > now()"
        " RETURNING id",
        3,
        NULL,
        parameter_values,
        NULL,
        NULL,
        0
    );

    int outcome = JOB_DATABASE_ERROR;
    if (result == NULL || PQresultStatus(result) != PGRES_TUPLES_OK ||
        PQntuples(result) > 1) {
        set_result_error(connection, result, error_message, error_message_size);
    } else {
        outcome = PQntuples(result) == 1
            ? JOB_DATABASE_LEASE_RENEWED
            : JOB_DATABASE_LEASE_LOST;
    }

    if (result != NULL) {
        PQclear(result);
    }
    PQfinish(connection);
    return outcome;
}

int job_database_finish(
    const char *connection_string,
    int64_t job_id,
    const char *worker_id,
    int exit_code,
    const char *error_text,
    char *error_message,
    size_t error_message_size
)
{
    if (error_message != NULL && error_message_size > 0) {
        error_message[0] = '\0';
    }
    if (worker_id == NULL || worker_id[0] == '\0') {
        set_error(error_message, error_message_size,
                  "Invalid job completion arguments");
        return JOB_DATABASE_ERROR;
    }

    PGconn *connection = connect_database(
        connection_string,
        error_message,
        error_message_size
    );
    if (connection == NULL) {
        return JOB_DATABASE_ERROR;
    }

    char id_text[32];
    char exit_text[32];
    snprintf(id_text, sizeof(id_text), "%" PRId64, job_id);
    snprintf(exit_text, sizeof(exit_text), "%d", exit_code);
    const char *parameter_values[] = {
        id_text,
        worker_id,
        exit_text,
        error_text
    };
    PGresult *result = PQexecParams(
        connection,
        "UPDATE jobs"
        " SET status = CASE WHEN $3::integer = 0"
        "                   THEN 'COMPLETED' ELSE 'FAILED' END,"
        "     exit_code = $3::integer, error_text = $4, finished_at = now(),"
        "     worker_id = NULL, lease_until = NULL"
        " WHERE id = $1 AND worker_id = $2 AND status = 'RUNNING'"
        " RETURNING id",
        4,
        NULL,
        parameter_values,
        NULL,
        NULL,
        0
    );

    int outcome = JOB_DATABASE_ERROR;
    if (result == NULL || PQresultStatus(result) != PGRES_TUPLES_OK ||
        PQntuples(result) > 1) {
        set_result_error(connection, result, error_message, error_message_size);
    } else {
        outcome = PQntuples(result) == 1 ? 1 : JOB_DATABASE_LEASE_LOST;
    }

    if (result != NULL) {
        PQclear(result);
    }
    PQfinish(connection);
    return outcome;
}

void job_database_job_clear(DatabaseJob *job)
{
    if (job == NULL) {
        return;
    }
    free(job->command);
    job->command = NULL;
    job->id = 0;
    job->attempts = 0;
}
