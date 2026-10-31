#ifndef QUEUE_H
#define QUEUE_H

#include "Job.h"

typedef struct JobNode {
    Job job;
    struct JobNode *next;
} JobNode;

typedef struct {
    JobNode *front;
    JobNode *rear;
    int size;
} JobQueue;

void queue_init(JobQueue *queue);
int queue_is_empty(const JobQueue *queue);
int queue_size(const JobQueue *queue);
int queue_enqueue(JobQueue *queue, Job job);
int queue_dequeue(JobQueue *queue, Job *job);
void queue_destroy(JobQueue *queue);

#endif