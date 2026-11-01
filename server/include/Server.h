#pragma once

#include "ThreadPool.h"
#include "Cache.h"

#include <atomic>
#include <string>
#include <thread>
#include <vector>

class Server {
public:
    void start();
    void stop();

private:
    void handleClient(int clientSocket);
    void workerLoop(int workerIndex);

    std::atomic<bool> running{true};
    Cache cache{1000};
    std::string databaseConnectionString;

    ThreadPool threadPool{4};
    std::vector<std::thread> workerThreads;
    static constexpr int WORKER_COUNT = 3;
};