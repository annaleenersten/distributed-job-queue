#pragma once

#include "ThreadPool.h"
#include "Cache.h"
#include "JobQueue.h"

#include <atomic>
#include <thread>
#include <chrono>

class Server {
public:
    void start();
    void stop();

private:
    void handleClient(int clientSocket);
    void workerLoop();

    ThreadPool threadPool{4};
    Cache cache{1000};
    JobQueueSystem* jobQueue{nullptr};
    std::thread workerThread;
    std::atomic<bool> running{true};
};