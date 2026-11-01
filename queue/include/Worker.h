#ifndef WORKER_H
#define WORKER_H

#include "Job.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int worker_execute(Job *job);
int worker_execute_command(
    int64_t job_id,
    const char *command,
    int *exit_code,
    int (*heartbeat)(void *context),
    void *context
);

#ifdef __cplusplus
}
#endif

#endif