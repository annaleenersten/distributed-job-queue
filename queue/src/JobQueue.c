#include <stdlib.h>

#include "JobQueue.h"
#include "Queue.h"
#include "JobStore.h"
#include "Worker.h"

struct JobQueueSystem {
    JobQueue queue;
    JobStore store;
};

JobQueueSystem *job_queue_create(void)
{
    JobQueueSystem *system = malloc(sizeof(JobQueueSystem));

    if (system == NULL) {
        return NULL;
    }

    queue_init(&system->queue);
    job_store_init(&system->store);

    return system;
}

void job_queue_destroy(JobQueueSystem *system)
{
    if (system == NULL) {
        return;
    }

    queue_destroy(&system->queue);
    job_store_destroy(&system->store);

    free(system);
}

int job_queue_submit(
    JobQueueSystem *system,
    const char *command
)
{
    if (system == NULL || command == NULL) {
        return 0;
    }

    int job_id = job_store_add(&system->store, command);

    if (job_id == 0) {
        return 0;
    }

    Job *job = job_store_get(&system->store, job_id);

    if (job == NULL) {
        return 0;
    }

    if (!queue_enqueue(&system->queue, *job)) {
        return 0;
    }

    return job_id;
}

Job *job_queue_get(
    JobQueueSystem *system,
    int job_id
)
{
    if (system == NULL) {
        return NULL;
    }

    return job_store_get(&system->store, job_id);
}

int job_queue_get_status(
    JobQueueSystem *system,
    int job_id
)
{
    Job *job = job_queue_get(system, job_id);

    if (job == NULL) {
        return -1;
    }

    return job->status;
}

const char *job_queue_get_command(
    JobQueueSystem *system,
    int job_id
)
{
    Job *job = job_queue_get(system, job_id);

    if (job == NULL) {
        return NULL;
    }

    return job->command;
}

int job_queue_process_one(JobQueueSystem *system)
{
    if (system == NULL) {
        return 0;
    }

    if (queue_is_empty(&system->queue)) {
        return 0;
    }

    worker_process_one(&system->queue, &system->store);

    return 1;
}