#pragma once

#include "ThreadPool.h"
#include "Cache.h"

#include <atomic>
#include <string>

class Server {
public:
    void start();
    void stop();

private:
    void handleClient(int clientSocket);

    std::atomic<bool> running{true};
    Cache cache{1000};
    std::string databaseConnectionString;

    ThreadPool threadPool{4};
};