#include "Server.h"
#include "HttpRequest.h"
#include "Router.h"
#include "HttpResponse.h"

#include <iostream>
#include <cstring>
#include <csignal>
#include <algorithm>
#include <sys/select.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

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
    Router router(cache, jobQueue);
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

    jobQueue = job_queue_create();

    if (jobQueue == nullptr) {
        std::cerr << "Failed to create job queue\n";
        return;
    }

    // Create a TCP socket
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (serverSocket == -1) {
        std::cerr << "Failed to create socket\n";
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

    close(serverSocket);

    job_queue_destroy(jobQueue);
    jobQueue = nullptr;
}

void Server::stop() {
    running = false;
}