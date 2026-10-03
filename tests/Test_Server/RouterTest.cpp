#include "Router.h"

#include <gtest/gtest.h>

#include <chrono>

TEST(RouterTest, HandlesRootRoute) {
    Cache cache{1000};
    Router router(cache);

    HttpRequest request(
        "GET / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n"
    );

    HttpResponse response = router.route(request);

    EXPECT_EQ(response.getStatusCode(), 200);
    EXPECT_EQ(response.getBody(), "Hello, World!");
}

TEST(RouterTest, HandlesHelloRoute) {
    Cache cache{1000};
    Router router(cache);

    HttpRequest request(
        "GET /hello HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n"
    );

    HttpResponse response = router.route(request);

    EXPECT_EQ(response.getStatusCode(), 200);
    EXPECT_EQ(response.getBody(), "Hello from the server!");
}

TEST(RouterTest, HandlesAboutRoute) {
    Cache cache{1000};
    Router router(cache);

    HttpRequest request(
        "GET /about HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n"
    );

    HttpResponse response = router.route(request);

    EXPECT_EQ(response.getStatusCode(), 200);
    EXPECT_EQ(response.getBody(), "C++ Concurrent Cache Server");
}

TEST(RouterTest, ReturnsNotFoundForUnknownRoute) {
    Cache cache{1000};
    Router router(cache);

    HttpRequest request(
        "GET /does-not-exist HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n"
    );

    HttpResponse response = router.route(request);

    EXPECT_EQ(response.getStatusCode(), 404);
}

TEST(RouterTest, StoresValueWithPut) {
    Cache cache{1000};
    Router router(cache);

    HttpRequest request(
        "PUT /cache/name HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Length: 7\r\n"
        "\r\n"
        "Annalee"
    );

    HttpResponse response = router.route(request);

    EXPECT_EQ(response.getStatusCode(), 200);

    auto value = cache.get("name");

    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(value.value(), "Annalee");
}

TEST(RouterTest, RetrievesValueWithGet) {
    Cache cache{1000};
    Router router(cache);

    cache.set(
        "name",
        "Annalee",
        std::chrono::seconds(60)
    );

    HttpRequest request(
        "GET /cache/name HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n"
    );

    HttpResponse response = router.route(request);

    EXPECT_EQ(response.getStatusCode(), 200);
    EXPECT_EQ(response.getBody(), "Annalee");
}

TEST(RouterTest, ReturnsNotFoundForMissingKey) {
    Cache cache{1000};
    Router router(cache);

    HttpRequest request(
        "GET /cache/missing HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n"
    );

    HttpResponse response = router.route(request);

    EXPECT_EQ(response.getStatusCode(), 404);
    EXPECT_EQ(response.getBody(), "Key not found");
}

TEST(RouterTest, DeletesValue) {
    Cache cache{1000};
    Router router(cache);

    cache.set(
        "name",
        "Annalee",
        std::chrono::seconds(60)
    );

    HttpRequest request(
        "DELETE /cache/name HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n"
    );

    HttpResponse response = router.route(request);

    EXPECT_EQ(response.getStatusCode(), 200);
    EXPECT_FALSE(cache.get("name").has_value());
}

TEST(RouterTest, ReturnsNotFoundWhenDeletingMissingKey) {
    Cache cache{1000}; 
    Router router(cache);

    HttpRequest request(
        "DELETE /cache/missing HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n"
    );

    HttpResponse response = router.route(request);

    EXPECT_EQ(response.getStatusCode(), 404);
}

TEST(RouterTest, ReturnsBadRequestForInvalidRequest) {
    Cache cache{1000};  
    Router router(cache);

    HttpRequest request(
        "GET\r\n"
        "\r\n"
    );

    HttpResponse response = router.route(request);

    EXPECT_EQ(response.getStatusCode(), 400);
    EXPECT_EQ(response.getBody(), "Invalid HTTP request");
}

TEST(RouterTest, ReturnsMethodNotAllowedForInvalidMethod) {
    Cache cache{1000};
    Router router(cache);

    HttpRequest request(
        "POST /hello HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n"
    );

    HttpResponse response = router.route(request);

    EXPECT_EQ(response.getStatusCode(), 405);
    EXPECT_EQ(response.getBody(), "Method not allowed");
}

TEST(RouterTest, ReturnsBadRequestForEmptyKey) {
    Cache cache{1000};
    Router router(cache);

    HttpRequest request(
        "GET /cache/ HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n"
    );

    HttpResponse response = router.route(request);

    EXPECT_EQ(response.getStatusCode(), 400);
    EXPECT_EQ(response.getBody(), "Key cannot be empty");
}

TEST(RouterTest, ReturnsCacheStats) {
    Cache cache(10);
    Router router(cache);

    HttpRequest request(
        "GET /stats HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n"
    );

    HttpResponse response = router.route(request);

    EXPECT_EQ(response.getStatusCode(), 200);
    EXPECT_EQ(response.getBody(), "Hits: 0\nMisses: 0");
}