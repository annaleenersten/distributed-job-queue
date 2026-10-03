#ifndef WORKER_H
#define WORKER_H

#include "Queue.h"
#include "JobStore.h"

void worker_process_one(JobQueue *queue, JobStore *store);
void worker_run(JobQueue *queue, JobStore *store);

#endif