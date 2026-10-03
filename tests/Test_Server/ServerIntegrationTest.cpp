#include <gtest/gtest.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <string>

std::string sendRequest(const std::string& request) {
    int socketFd = socket(AF_INET, SOCK_STREAM, 0);

    EXPECT_NE(socketFd, -1);

    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);

    inet_pton(
        AF_INET,
        "127.0.0.1",
        &serverAddress.sin_addr
    );

    EXPECT_EQ(
        connect(
            socketFd,
            reinterpret_cast<sockaddr*>(&serverAddress),
            sizeof(serverAddress)
        ),
        0
    );

    ssize_t bytesSent = send(
        socketFd,
        request.c_str(),
        request.size(),
        0
    );

    EXPECT_EQ(
        bytesSent,
        static_cast<ssize_t>(request.size())
    );

    char buffer[4096];
    std::string response;

    while (true) {
        ssize_t bytesReceived = recv(
            socketFd,
            buffer,
            sizeof(buffer),
            0
        );

        if (bytesReceived <= 0) {
            break;
        }

        response.append(buffer, bytesReceived);
    }

    close(socketFd);

    return response;
}

TEST(ServerIntegrationTest, PutThenGet) {
    std::string putRequest =
        "PUT /cache/test HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Length: 5\r\n"
        "\r\n"
        "Hello";

    std::string putResponse = sendRequest(putRequest);

    EXPECT_NE(
        putResponse.find("200 OK"),
        std::string::npos
    );

    std::string getRequest =
        "GET /cache/test HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";

    std::string getResponse = sendRequest(getRequest);

    EXPECT_NE(
        getResponse.find("200 OK"),
        std::string::npos
    );

    EXPECT_NE(
        getResponse.find("Hello"),
        std::string::npos
    );
}

TEST(ServerIntegrationTest, GetMissingKeyReturns404) {
    std::string getRequest =
        "GET /cache/missing HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";

    std::string response = sendRequest(getRequest);

    EXPECT_NE(
        response.find("404 Not Found"),
        std::string::npos
    );
}

TEST(ServerIntegrationTest, DeleteThenGetReturns404) {
    std::string putRequest =
        "PUT /cache/delete-test HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Length: 5\r\n"
        "\r\n"
        "Hello";

    std::string putResponse = sendRequest(putRequest);

    EXPECT_NE(
        putResponse.find("200 OK"),
        std::string::npos
    );

    std::string deleteRequest =
        "DELETE /cache/delete-test HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";

    std::string deleteResponse = sendRequest(deleteRequest);

    EXPECT_NE(
        deleteResponse.find("200 OK"),
        std::string::npos
    );

    std::string getRequest =
        "GET /cache/delete-test HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";

    std::string getResponse = sendRequest(getRequest);

    EXPECT_NE(
        getResponse.find("404 Not Found"),
        std::string::npos
    );
}

TEST(ServerIntegrationTest, UnsupportedMethodReturns405) {
    std::string request =
        "POST /cache/test HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";

    std::string response = sendRequest(request);

    EXPECT_NE(
        response.find("405 Method Not Allowed"),
        std::string::npos
    );
}

TEST(ServerIntegrationTest, HandlesLargeRequestBody) {
    std::string largeBody(10000, 'A');

    std::string request =
        "PUT /cache/large HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Length: " +
        std::to_string(largeBody.size()) +
        "\r\n"
        "\r\n" +
        largeBody;

    std::string putResponse = sendRequest(request);

    EXPECT_NE(
        putResponse.find("200 OK"),
        std::string::npos
    );

    std::string getRequest =
        "GET /cache/large HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";

    std::string getResponse = sendRequest(getRequest);

    EXPECT_NE(
        getResponse.find("200 OK"),
        std::string::npos
    );

    EXPECT_NE(
        getResponse.find(largeBody),
        std::string::npos
    );
}

TEST(ServerIntegrationTest, HandlesMultipleClients) {
    std::string request1 =
        "PUT /cache/client1 HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Length: 5\r\n"
        "\r\n"
        "One!!";

    std::string request2 =
        "PUT /cache/client2 HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Length: 5\r\n"
        "\r\n"
        "Two!!";

    std::string response1 = sendRequest(request1);
    std::string response2 = sendRequest(request2);

    EXPECT_NE(
        response1.find("200 OK"),
        std::string::npos
    );

    EXPECT_NE(
        response2.find("200 OK"),
        std::string::npos
    );

    std::string getRequest1 =
        "GET /cache/client1 HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";

    std::string getRequest2 =
        "GET /cache/client2 HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";

    std::string getResponse1 = sendRequest(getRequest1);
    std::string getResponse2 = sendRequest(getRequest2);

    EXPECT_NE(
        getResponse1.find("One!!"),
        std::string::npos
    );

    EXPECT_NE(
        getResponse2.find("Two!!"),
        std::string::npos
    );
}

TEST(ServerIntegrationTest, EmptyCacheKeyReturns400) {
    std::string request =
        "GET /cache/ HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";

    std::string response = sendRequest(request);

    EXPECT_NE(
        response.find("400 Bad Request"),
        std::string::npos
    );
}

TEST(ServerIntegrationTest, UnknownRouteReturns404) {
    std::string request =
        "GET /does-not-exist HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";

    std::string response = sendRequest(request);

    EXPECT_NE(
        response.find("404 Not Found"),
        std::string::npos
    );
}

TEST(ServerIntegrationTest, InvalidHttpVersionReturns400) {
    std::string request =
        "GET /hello HTTP/2.0\r\n"
        "Host: localhost\r\n"
        "\r\n";

    std::string response = sendRequest(request);

    EXPECT_NE(
        response.find("400 Bad Request"),
        std::string::npos
    );
}

TEST(ServerIntegrationTest, MalformedRequestReturns400) {
    std::string request =
        "GET\r\n"
        "\r\n";

    std::string response = sendRequest(request);

    EXPECT_NE(
        response.find("400 Bad Request"),
        std::string::npos
    );
}

TEST(ServerIntegrationTest, StoresEmptyValue) {
    std::string putRequest =
        "PUT /cache/empty HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Length: 0\r\n"
        "\r\n";

    std::string putResponse = sendRequest(putRequest);

    EXPECT_NE(
        putResponse.find("200 OK"),
        std::string::npos
    );

    std::string getRequest =
        "GET /cache/empty HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";

    std::string getResponse = sendRequest(getRequest);

    EXPECT_NE(
        getResponse.find("200 OK"),
        std::string::npos
    );
}

TEST(ServerIntegrationTest, UpdatesExistingValue) {
    std::string firstPut =
        "PUT /cache/update HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Length: 3\r\n"
        "\r\n"
        "Old";

    std::string firstResponse = sendRequest(firstPut);

    EXPECT_NE(
        firstResponse.find("200 OK"),
        std::string::npos
    );

    std::string secondPut =
        "PUT /cache/update HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Length: 3\r\n"
        "\r\n"
        "New";

    std::string secondResponse = sendRequest(secondPut);

    EXPECT_NE(
        secondResponse.find("200 OK"),
        std::string::npos
    );

    std::string getRequest =
        "GET /cache/update HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";

    std::string getResponse = sendRequest(getRequest);

    EXPECT_NE(
        getResponse.find("New"),
        std::string::npos
    );

    EXPECT_EQ(
        getResponse.find("Old"),
        std::string::npos
    );
}