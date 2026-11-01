#include "Server.h"
#include "HttpRequest.h"
#include "Router.h"
#include "HttpResponse.h"
#include "JobDatabase.h"
#include "Worker.h"

#include <iostream>
#include <cstring>
#include <csignal>
#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <chrono>
#include <cstdint>
#include <string>
#include <thread>
#include <sys/select.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

namespace {

constexpr int JOB_LEASE_SECONDS = 90;

struct JobLeaseContext {
    const std::string* connectionString;
    std::string workerId;
    int64_t jobId;
};

int renewJobLease(void* context) {
    auto* lease = static_cast<JobLeaseContext*>(context);
    char errorMessage[512];
    int result = job_database_renew_lease(
        lease->connectionString->c_str(),
        lease->jobId,
        lease->workerId.c_str(),
        JOB_LEASE_SECONDS,
        errorMessage,
        sizeof(errorMessage)
    );

    if (result == JOB_DATABASE_ERROR) {
        std::cerr << "Failed to renew job " << lease->jobId
                  << " lease: " << errorMessage << '\n';
    }

    return result == JOB_DATABASE_LEASE_RENEWED;
}

}

Server* serverInstance = nullptr;

void handleSignal(int signal) {
    if (signal == SIGINT && serverInstance != nullptr) {
        serverInstance->stop();
    }
}

void Server::handleClient(int clientSocket) {
    // Receive request
    std::string requestData;
    char buffer[4096];

    while (true) {
        ssize_t bytesReceived = recv(
            clientSocket,
            buffer,
            sizeof(buffer),
            0
        );

        if (bytesReceived <= 0) {
            break;
        }

        requestData.append(buffer, bytesReceived);

        // Stop once we've received the end of the headers.
        if (requestData.find("\r\n\r\n") != std::string::npos) {
            break;
        }
    }

    if (requestData.empty()) {
        std::cerr << "Failed to receive data\n";
        close(clientSocket);
        return;
    }

    std::size_t headerEnd = requestData.find("\r\n\r\n");
    std::size_t bodyStart = headerEnd + 4;
    std::size_t bodyBytesReceived = requestData.size() - bodyStart; 

    // Parse HTTP request
    HttpRequest request(requestData);
    std::size_t contentLength = request.getContentLength();

    while (bodyBytesReceived < contentLength) {
        std::size_t remaining = contentLength - bodyBytesReceived;

        ssize_t bytesReceived = recv(
            clientSocket,
            buffer,
            std::min(sizeof(buffer), remaining),
            0
        );

        if (bytesReceived <= 0) {
            break;
        }

        requestData.append(buffer, bytesReceived);
        bodyBytesReceived += bytesReceived;
    }

    if (bodyBytesReceived < contentLength) {
        std::cerr << "Incomplete request body\n";
        close(clientSocket);
        return;
    }

    // Re-parse the complete request.
    request = HttpRequest(requestData);

    std::cout << "Received:\n";
    std::cout << requestData << '\n';

    std::cout << "Method: " << request.getMethod() << '\n';
    std::cout << "Path: " << request.getPath() << '\n';
    std::cout << "Version: " << request.getVersion() << '\n';

    // Route request
    Router router(cache, databaseConnectionString);
    HttpResponse response = router.route(request);

    // Build HTTP response
    std::string responseData =
        "HTTP/1.1 " +
        std::to_string(response.getStatusCode()) + " " +
        response.getStatusText() + "\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: " +
        std::to_string(response.getBody().size()) + "\r\n"
        "\r\n" +
        response.getBody();

    // Send response
    ssize_t bytesSent = send(
        clientSocket,
        responseData.c_str(),
        responseData.size(),
        0
    );

    if (bytesSent == -1) {
        std::cerr << "Failed to send response\n";
    }

    close(clientSocket);
}

void Server::start() {
    serverInstance = this;
    std::signal(SIGINT, handleSignal);

    const char* connectionString = std::getenv("DATABASE_URL");
    if (connectionString == nullptr || connectionString[0] == '\0') {
        std::cerr << "DATABASE_URL is not set; job submissions will fail\n";
    } else {
        databaseConnectionString = connectionString;
    }

    // Create a TCP socket
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (serverSocket == -1) {
        std::cerr << "Failed to create socket\n";
        return;
    }

    int reuseAddress = 1;
    if (setsockopt(
            serverSocket,
            SOL_SOCKET,
            SO_REUSEADDR,
            &reuseAddress,
            sizeof(reuseAddress)) == -1) {
        std::cerr << "Failed to configure socket address reuse\n";
        close(serverSocket);
        return;
    }

    // Configure the server address
    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(8080);

    // Bind the socket to port 8080
    if (bind(
            serverSocket,
            reinterpret_cast<sockaddr*>(&serverAddress),
            sizeof(serverAddress)) == -1) {

        std::cerr << "Failed to bind socket\n";
        close(serverSocket);
        return;
    }

    // Start listening for connections
    if (listen(serverSocket, 10) == -1) {
        std::cerr << "Failed to listen on socket\n";
        close(serverSocket);
        return;
    }

    if (!databaseConnectionString.empty()) {
        for (int i = 0; i < WORKER_COUNT; ++i) {
            workerThreads.emplace_back(&Server::workerLoop, this, i);
        }
    }

    std::cout << "Server listening on port 8080...\n";

    // Accept clients
    while (running) {
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(serverSocket, &readSet);

        timeval timeout{};
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        int ready = select(
            serverSocket + 1,
            &readSet,
            nullptr,
            nullptr,
            &timeout
        );

        if (ready == -1) {
            if (!running) {
                break;
            }

            std::cerr << "Failed to monitor socket\n";
            break;
        }

        if (ready == 0) {
            continue;
        }

        sockaddr_in clientAddress{};
        socklen_t clientAddressLength = sizeof(clientAddress);

        int clientSocket = accept(
            serverSocket,
            reinterpret_cast<sockaddr*>(&clientAddress),
            &clientAddressLength
        );

        if (clientSocket == -1) {
            std::cerr << "Failed to accept connection\n";
            continue;
        }

        std::cout << "Client connected!\n";

        threadPool.enqueue([this, clientSocket]() {
            handleClient(clientSocket);
        });
    }

    running = false;
    close(serverSocket);

    for (auto& worker : workerThreads) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

void Server::stop() {
    running = false;
}

void Server::workerLoop(int workerIndex) {
    char hostname[64] = "worker";
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        hostname[sizeof(hostname) - 1] = '\0';
    }

    const std::string workerId =
        std::string(hostname) + "-" + std::to_string(getpid()) + "-" +
        std::to_string(workerIndex);

    while (running) {
        DatabaseJob job{};
        char errorMessage[512];
        int claimResult = job_database_claim(
            databaseConnectionString.c_str(),
            workerId.c_str(),
            JOB_LEASE_SECONDS,
            &job,
            errorMessage,
            sizeof(errorMessage)
        );

        if (claimResult == JOB_DATABASE_ERROR) {
            std::cerr << "Worker " << workerId
                      << " failed to claim a job: " << errorMessage << '\n';
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }
        if (claimResult == JOB_DATABASE_NO_JOB) {
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            continue;
        }

        JobLeaseContext lease{&databaseConnectionString, workerId, job.id};
        int exitCode = -1;
        const int executed = worker_execute_command(
            job.id,
            job.command,
            &exitCode,
            renewJobLease,
            &lease
        );

        std::string errorText;
        if (!executed) {
            errorText = "Worker failed or lost its database lease";
        } else if (exitCode != 0) {
            errorText = "Command exited with status " + std::to_string(exitCode);
        }

        char finishError[512];
        int finishResult = job_database_finish(
            databaseConnectionString.c_str(),
            job.id,
            workerId.c_str(),
            exitCode,
            errorText.empty() ? nullptr : errorText.c_str(),
            finishError,
            sizeof(finishError)
        );
        if (finishResult == JOB_DATABASE_ERROR) {
            std::cerr << "Worker " << workerId << " failed to finish job "
                      << job.id << ": " << finishError << '\n';
        } else if (finishResult == JOB_DATABASE_LEASE_LOST) {
            std::cerr << "Worker " << workerId
                      << " no longer owns job " << job.id << '\n';
        }

        job_database_job_clear(&job);
    }
}