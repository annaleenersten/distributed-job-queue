#ifndef JOB_QUEUE_H
#define JOB_QUEUE_H

#include "Job.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct JobQueueSystem JobQueueSystem;

JobQueueSystem *job_queue_create(void);
void job_queue_destroy(JobQueueSystem *system);

int job_queue_submit(JobQueueSystem *system, const char *command);

int job_queue_get(JobQueueSystem *system, int job_id, Job *job);

int job_queue_process_one(JobQueueSystem *system);

void job_queue_shutdown(JobQueueSystem *system);

#ifdef __cplusplus
}
#endif

#endif