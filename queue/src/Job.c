#include <string.h>

#include "Job.h"

Job job_create(int id, const char *command)
{
    Job job;

    job.id = id;
    job.status = JOB_QUEUED;

    strncpy(job.command, command, sizeof(job.command) - 1);
    job.command[sizeof(job.command) - 1] = '\0';

    return job;
}