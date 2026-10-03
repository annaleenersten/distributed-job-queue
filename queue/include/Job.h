#ifndef JOB_H
#define JOB_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    JOB_QUEUED,
    JOB_RUNNING,
    JOB_COMPLETED,
    JOB_FAILED
} JobStatus;

typedef struct {
    int id;
    char command[256];
    JobStatus status;
} Job;

Job job_create(int id, const char *command);

#ifdef __cplusplus
}
#endif

#endif