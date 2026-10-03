#include <stddef.h>

#include "JobStore.h"

void job_store_init(JobStore *store)
{
    store->count = 0;
    store->next_id = 1;
}

int job_store_add(JobStore *store, const char *command)
{
    if (store->count >= MAX_JOBS) {
        return 0;
    }

    Job job = job_create(store->next_id, command);

    store->jobs[store->count] = job;
    store->count++;
    store->next_id++;

    return job.id;
}

Job *job_store_get(JobStore *store, int job_id)
{
    for (int i = 0; i < store->count; i++) {
        if (store->jobs[i].id == job_id) {
            return &store->jobs[i];
        }
    }

    return NULL;
}

int job_store_update_status(JobStore *store, int job_id, JobStatus status)
{
    Job *job = job_store_get(store, job_id);

    if (job == NULL) {
        return 0;
    }

    job->status = status;

    return 1;
}

void job_store_destroy(JobStore *store)
{
    store->count = 0;
    store->next_id = 1;
}