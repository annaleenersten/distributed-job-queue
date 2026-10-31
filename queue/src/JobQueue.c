#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>

#include "JobQueue.h"
#include "Queue.h"
#include "JobStore.h"
#include "Worker.h"

struct JobQueueSystem {
    JobQueue queue;
    JobStore store;
    pthread_mutex_t mutex;
    pthread_cond_t condition;
    int shutting_down;
};

JobQueueSystem *job_queue_create(void)
{
    JobQueueSystem *system = malloc(sizeof(JobQueueSystem));

    if (system == NULL) {
        return NULL;
    }

    queue_init(&system->queue);
    job_store_init(&system->store);

    if (pthread_mutex_init(&system->mutex, NULL) != 0) {
        free(system);
        return NULL;
    }

    if (pthread_cond_init(&system->condition, NULL) != 0) {
        pthread_mutex_destroy(&system->mutex);
        free(system);
        return NULL;
    }

    system->shutting_down = 0;

    return system;
}

void job_queue_destroy(JobQueueSystem *system)
{
    if (system == NULL) {
        return;
    }

    queue_destroy(&system->queue);
    job_store_destroy(&system->store);

    pthread_cond_destroy(&system->condition);
    pthread_mutex_destroy(&system->mutex);

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

    pthread_mutex_lock(&system->mutex);

    int job_id = job_store_add(&system->store, command);

    if (job_id == 0) {
        pthread_mutex_unlock(&system->mutex);
        return 0;
    }

    Job *job = job_store_get(&system->store, job_id);

    if (job == NULL) {
        pthread_mutex_unlock(&system->mutex);
        return 0;
    }

    if (!queue_enqueue(&system->queue, *job)) {
        pthread_mutex_unlock(&system->mutex);
        return 0;
    }

    pthread_cond_signal(&system->condition);

    pthread_mutex_unlock(&system->mutex);

    return job_id;
}

int job_queue_get(
    JobQueueSystem *system,
    int job_id,
    Job *job
)
{
    if (system == NULL || job == NULL) {
        return 0;
    }

    pthread_mutex_lock(&system->mutex);

    Job *stored_job = job_store_get(&system->store, job_id);

    if (stored_job == NULL) {
        pthread_mutex_unlock(&system->mutex);
        return 0;
    }

    *job = *stored_job;

    pthread_mutex_unlock(&system->mutex);

    return 1;
}

int job_queue_process_one(JobQueueSystem *system)
{
    if (system == NULL) {
        return 0;
    }

    Job job;

    /*
     * Get a job and mark it as running while holding
     * the mutex.
     */
    pthread_mutex_lock(&system->mutex);

    while (queue_is_empty(&system->queue) && !system->shutting_down) {
        pthread_cond_wait(&system->condition, &system->mutex);
    }

    if (system->shutting_down && queue_is_empty(&system->queue)) {
        pthread_mutex_unlock(&system->mutex);
        return 0;
    }

    queue_dequeue(&system->queue, &job);

    if (!job_store_update_status(
            &system->store,
            job.id,
            JOB_RUNNING)) {

        pthread_mutex_unlock(&system->mutex);
        return 0;
    }

    pthread_mutex_unlock(&system->mutex);

    /*
     * Execute the job without holding the mutex.
     */
    int success = worker_execute(&job);

    /*
     * Update the job status after execution.
     */
    pthread_mutex_lock(&system->mutex);

    if (success) {
        job_store_update_status(
            &system->store,
            job.id,
            JOB_COMPLETED
        );
    } else {
        job_store_update_status(
            &system->store,
            job.id,
            JOB_FAILED
        );
    }

    pthread_mutex_unlock(&system->mutex);

    if (success) {
        printf("Job %d completed.\n", job.id);
    } else {
        printf("Job %d failed.\n", job.id);
    }

    return 1;
}

void job_queue_shutdown(JobQueueSystem *system)
{
    if (system == NULL) {
        return;
    }

    pthread_mutex_lock(&system->mutex);

    system->shutting_down = 1;

    pthread_cond_broadcast(&system->condition);

    pthread_mutex_unlock(&system->mutex);
}