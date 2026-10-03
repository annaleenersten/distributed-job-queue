#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

#include "Worker.h"

void worker_process_one(JobQueue *queue, JobStore *store)
{
    Job job;

    if (!queue_dequeue(queue, &job)) {
        printf("No jobs in queue.\n");
        return;
    }

    // Mark the job as running in the store.
    if (!job_store_update_status(store, job.id, JOB_RUNNING)) {
        printf("Could not update job %d status.\n", job.id);
        return;
    }

    printf("Worker processing job %d: %s\n", job.id, job.command);

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        job_store_update_status(store, job.id, JOB_FAILED);
        return;
    }

    if (pid == 0) {
        execl("/bin/sh", "sh", "-c", job.command, (char *)NULL);

        perror("execl");
        _exit(1);
    }

    int status;

    if (waitpid(pid, &status, 0) == -1) {
        perror("waitpid");
        job_store_update_status(store, job.id, JOB_FAILED);
        return;
    }

    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        job_store_update_status(store, job.id, JOB_COMPLETED);
        printf("Job %d completed.\n", job.id);
    } else {
        job_store_update_status(store, job.id, JOB_FAILED);
        printf("Job %d failed.\n", job.id);
    }
}

void worker_run(JobQueue *queue, JobStore *store)
{
    while (!queue_is_empty(queue)) {
        worker_process_one(queue, store);
    }
}