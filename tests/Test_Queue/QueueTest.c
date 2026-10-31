#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#include "JobQueue.h"

typedef struct {
    JobQueueSystem *system;
} WorkerArgs;

void *test_worker(void *arg)
{
    WorkerArgs *args = (WorkerArgs *)arg;

    assert(job_queue_process_one(args->system));

    return NULL;
}

int main(void)
{
    /*
     * Basic job queue tests.
     */
    JobQueueSystem *system = job_queue_create();

    assert(system != NULL);

    // Submit jobs.
    int id1 = job_queue_submit(system, "echo hello");
    int id2 = job_queue_submit(system, "false");

    assert(id1 == 1);
    assert(id2 == 2);

    // Retrieve submitted jobs.
    Job job1;
    Job job2;

    assert(job_queue_get(system, id1, &job1));
    assert(job_queue_get(system, id2, &job2));

    assert(job1.id == id1);
    assert(strcmp(job1.command, "echo hello") == 0);
    assert(job1.status == JOB_QUEUED);

    assert(job2.id == id2);
    assert(strcmp(job2.command, "false") == 0);
    assert(job2.status == JOB_QUEUED);

    // Test missing job.
    Job missing_job;

    assert(!job_queue_get(system, 999, &missing_job));

    // Process the first job successfully.
    assert(job_queue_process_one(system));

    assert(job_queue_get(system, id1, &job1));
    assert(job1.status == JOB_COMPLETED);

    // Process the second job, which should fail.
    assert(job_queue_process_one(system));

    assert(job_queue_get(system, id2, &job2));
    assert(job2.status == JOB_FAILED);

    job_queue_destroy(system);

    /*
     * Multiple-worker concurrency test.
     */
    system = job_queue_create();

    assert(system != NULL);

    int job_ids[3];

    job_ids[0] = job_queue_submit(system, "sleep 2");
    job_ids[1] = job_queue_submit(system, "sleep 2");
    job_ids[2] = job_queue_submit(system, "sleep 2");

    assert(job_ids[0] == 1);
    assert(job_ids[1] == 2);
    assert(job_ids[2] == 3);

    pthread_t workers[3];

    WorkerArgs args = {
        .system = system
    };

    struct timespec start;
    struct timespec end;

    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int i = 0; i < 3; i++) {
        assert(pthread_create(
            &workers[i],
            NULL,
            test_worker,
            &args
        ) == 0);
    }

    for (int i = 0; i < 3; i++) {
        assert(pthread_join(workers[i], NULL) == 0);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    double elapsed =
        (end.tv_sec - start.tv_sec) +
        (end.tv_nsec - start.tv_nsec) / 1000000000.0;

    // Three 2-second jobs should finish in roughly 2 seconds,
    // rather than roughly 6 seconds with one worker.
    assert(elapsed < 4.0);

    for (int i = 0; i < 3; i++) {
        Job job;

        assert(job_queue_get(
            system,
            job_ids[i],
            &job
        ));

        assert(job.status == JOB_COMPLETED);
    }

    job_queue_destroy(system);

    printf("All queue tests passed!\n");

    return 0;
}