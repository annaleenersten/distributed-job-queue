#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "JobQueue.h"

int main(void)
{
    JobQueueSystem *system = job_queue_create();

    assert(system != NULL);

    // Submit jobs.
    int id1 = job_queue_submit(system, "echo hello");
    int id2 = job_queue_submit(system, "false");

    assert(id1 == 1);
    assert(id2 == 2);

    // Retrieve submitted jobs.
    Job *job1 = job_queue_get(system, id1);
    Job *job2 = job_queue_get(system, id2);

    assert(job1 != NULL);
    assert(job2 != NULL);

    assert(job1->id == id1);
    assert(strcmp(job1->command, "echo hello") == 0);
    assert(job1->status == JOB_QUEUED);

    assert(job2->id == id2);
    assert(strcmp(job2->command, "false") == 0);
    assert(job2->status == JOB_QUEUED);

    // Test status and command accessors.
    assert(job_queue_get_status(system, id1) == JOB_QUEUED);
    assert(job_queue_get_status(system, id2) == JOB_QUEUED);

    assert(strcmp(
        job_queue_get_command(system, id1),
        "echo hello"
    ) == 0);

    assert(strcmp(
        job_queue_get_command(system, id2),
        "false"
    ) == 0);

    // Test missing job.
    assert(job_queue_get(system, 999) == NULL);
    assert(job_queue_get_status(system, 999) == -1);
    assert(job_queue_get_command(system, 999) == NULL);

    // Process the first job successfully.
    assert(job_queue_process_one(system));

    job1 = job_queue_get(system, id1);

    assert(job1 != NULL);
    assert(job1->status == JOB_COMPLETED);

    // Process the second job, which should fail.
    assert(job_queue_process_one(system));

    job2 = job_queue_get(system, id2);

    assert(job2 != NULL);
    assert(job2->status == JOB_FAILED);

    // Queue should now be empty.
    assert(!job_queue_process_one(system));

    job_queue_destroy(system);

    printf("All queue tests passed!\n");

    return 0;
}