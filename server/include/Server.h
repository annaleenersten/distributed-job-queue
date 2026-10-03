#pragma once

#include "ThreadPool.h"
#include "Cache.h"
#include "JobQueue.h"

#include <atomic>

class Server {
public:
    void start();
    void stop();

private:
    void handleClient(int clientSocket);

    ThreadPool threadPool{4};
    Cache cache{1000};
    JobQueueSystem* jobQueue{nullptr};
    std::atomic<bool> running{true};
};