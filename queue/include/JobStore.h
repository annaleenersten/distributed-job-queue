#ifndef JOB_STORE_H
#define JOB_STORE_H

#include "Job.h"

#define MAX_JOBS 100

typedef struct {
    Job jobs[MAX_JOBS];
    int count;
    int next_id;
} JobStore;

void job_store_init(JobStore *store);

int job_store_add(JobStore *store, const char *command);

Job *job_store_get(JobStore *store, int job_id);

int job_store_update_status(JobStore *store, int job_id, JobStatus status);

void job_store_destroy(JobStore *store);

#endif