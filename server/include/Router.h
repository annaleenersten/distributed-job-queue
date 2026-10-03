#pragma once

#include "HttpRequest.h"
#include "HttpResponse.h"
#include "Cache.h"
#include "JobQueue.h"

class Router {
public:
    Router(Cache& cache, JobQueueSystem* jobQueue = nullptr);

    HttpResponse route(const HttpRequest& request) const;

private:
    Cache& cache;
    JobQueueSystem* jobQueue;
};