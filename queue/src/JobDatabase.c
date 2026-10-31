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

    if (connection_string == NULL || connection_string[0] == '\0') {
        set_error(error_message, error_message_size,
                  "Database connection string is empty");
        return 0;
    }

    if (command == NULL || command[0] == '\0') {
        set_error(error_message, error_message_size,
                  "Job command is empty");
        return 0;
    }

    if (job_id == NULL) {
        set_error(error_message, error_message_size,
                  "Job ID output pointer is NULL");
        return 0;
    }

    size_t command_length = strlen(command);
    if (command_length > INT_MAX) {
        set_error(error_message, error_message_size,
                  "Job command is too long");
        return 0;
    }

    PGconn *connection = PQconnectdb(connection_string);
    if (connection == NULL) {
        set_error(error_message, error_message_size,
                  "Could not allocate a PostgreSQL connection");
        return 0;
    }

    if (PQstatus(connection) != CONNECTION_OK) {
        set_error(error_message, error_message_size, PQerrorMessage(connection));
        PQfinish(connection);
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

    if (result == NULL || PQresultStatus(result) != PGRES_TUPLES_OK ||
        PQntuples(result) != 1 || PQnfields(result) != 1) {
        const char *message = PQerrorMessage(connection);
        if (result != NULL && PQresultErrorMessage(result)[0] != '\0') {
            message = PQresultErrorMessage(result);
        }
        set_error(error_message, error_message_size, message);
        if (result != NULL) {
            PQclear(result);
        }
        PQfinish(connection);
        return 0;
    }

    errno = 0;
    char *end = NULL;
    intmax_t parsed_id = strtoimax(PQgetvalue(result, 0, 0), &end, 10);
    if (errno != 0 || end == PQgetvalue(result, 0, 0) || *end != '\0' ||
        parsed_id < INT64_MIN || parsed_id > INT64_MAX) {
        set_error(error_message, error_message_size,
                  "PostgreSQL returned an invalid job ID");
        PQclear(result);
        PQfinish(connection);
        return 0;
    }

    *job_id = (int64_t)parsed_id;

    PQclear(result);
    PQfinish(connection);
    return 1;
}
