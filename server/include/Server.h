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

    std::atomic<bool> running{true};
    Cache cache{1000};
    JobQueueSystem* jobQueue{nullptr};

    ThreadPool threadPool{4};

    std::vector<std::thread> workerThreads;
    static constexpr int WORKER_COUNT = 3;  
};