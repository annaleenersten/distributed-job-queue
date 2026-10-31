#include <stdlib.h>
#include "Queue.h"

void queue_init(JobQueue *queue)
{
    queue->front = NULL;
    queue->rear = NULL;
    queue->size = 0;
}

int queue_is_empty(const JobQueue *queue)
{
    return queue->front == NULL;
}

int queue_size(const JobQueue *queue)
{
    return queue->size;
}

int queue_enqueue(JobQueue *queue, Job job)
{
    JobNode *new_node = malloc(sizeof(JobNode));

    if (new_node == NULL) {
        return 0;
    }

    new_node->job = job;
    new_node->next = NULL;

    if (queue_is_empty(queue)) {
        queue->front = new_node;
        queue->rear = new_node;
    } else {
        queue->rear->next = new_node;
        queue->rear = new_node;
    }

    queue->size++;

    return 1;
}

int queue_dequeue(JobQueue *queue, Job *job)
{
    if (queue_is_empty(queue)) {
        return 0;
    }

    JobNode *old_front = queue->front;

    *job = old_front->job;
    queue->front = old_front->next;

    if (queue->front == NULL) {
        queue->rear = NULL;
    }

    free(old_front);
    queue->size--;

    return 1;
}

void queue_destroy(JobQueue *queue)
{
    JobNode *current = queue->front;

    while (current != NULL) {
        JobNode *next = current->next;
        free(current);
        current = next;
    }

    queue->front = NULL;
    queue->rear = NULL;
    queue->size = 0;
}