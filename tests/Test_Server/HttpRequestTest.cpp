#include "HttpRequest.h"

#include <gtest/gtest.h>

TEST(HttpRequestTest, ParsesRequestLine) {
    std::string request =
        "GET /hello HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";

    HttpRequest httpRequest(request);

    EXPECT_EQ(httpRequest.getMethod(), "GET");
    EXPECT_EQ(httpRequest.getPath(), "/hello");
    EXPECT_EQ(httpRequest.getVersion(), "HTTP/1.1");
    EXPECT_TRUE(httpRequest.isValid());
}

TEST(HttpRequestTest, ParsesHeaders) {
    std::string request =
        "GET /hello HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Type: text/plain\r\n"
        "\r\n";

    HttpRequest httpRequest(request);

    EXPECT_EQ(httpRequest.getHeader("Host"), "localhost");
    EXPECT_EQ(httpRequest.getHeader("Content-Type"), "text/plain");
}

TEST(HttpRequestTest, ParsesRequestBody) {
    std::string request =
        "PUT /data/name HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Length: 7\r\n"
        "\r\n"
        "Annalee";

    HttpRequest httpRequest(request);

    EXPECT_EQ(httpRequest.getMethod(), "PUT");
    EXPECT_EQ(httpRequest.getPath(), "/data/name");
    EXPECT_EQ(httpRequest.getBody(), "Annalee");
    EXPECT_EQ(httpRequest.getHeader("Content-Length"), "7");
    EXPECT_TRUE(httpRequest.isValid());
}

TEST(HttpRequestTest, HandlesRequestWithoutBody) {
    std::string request =
        "GET /about HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";

    HttpRequest httpRequest(request);

    EXPECT_EQ(httpRequest.getBody(), "");
    EXPECT_TRUE(httpRequest.isValid());
}

TEST(HttpRequestTest, ReturnsEmptyForMissingHeader) {
    std::string request =
        "GET /hello HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";

    HttpRequest httpRequest(request);

    EXPECT_EQ(httpRequest.getHeader("Content-Type"), "");
}

TEST(HttpRequestTest, RejectsMalformedRequestLine) {
    std::string request =
        "GET\r\n"
        "\r\n";

    HttpRequest httpRequest(request);

    EXPECT_FALSE(httpRequest.isValid());
}

TEST(HttpRequestTest, RejectsUnsupportedHttpVersion) {
    std::string request =
        "GET /hello HTTP/2.0\r\n"
        "Host: localhost\r\n"
        "\r\n";

    HttpRequest httpRequest(request);

    EXPECT_FALSE(httpRequest.isValid());
}

TEST(HttpRequestTest, RejectsMalformedHeader) {
    std::string request =
        "GET /hello HTTP/1.1\r\n"
        "This is not a valid header\r\n"
        "\r\n";

    HttpRequest httpRequest(request);

    EXPECT_FALSE(httpRequest.isValid());
}

TEST(HttpRequestTest, ParsesContentLength) {
    std::string request =
        "PUT /data/name HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Length: 7\r\n"
        "\r\n"
        "Annalee";

    HttpRequest httpRequest(request);

    EXPECT_EQ(httpRequest.getContentLength(), 7);
}