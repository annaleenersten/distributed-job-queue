#include "HttpResponse.h"

#include <gtest/gtest.h>

TEST(HttpResponseTest, StoresStatusInformation) {
    HttpResponse response(200, "OK", "Hello, World!");

    EXPECT_EQ(response.getStatusCode(), 200);
    EXPECT_EQ(response.getStatusText(), "OK");
    EXPECT_EQ(response.getBody(), "Hello, World!");
}

TEST(HttpResponseTest, StoresNotFoundResponse) {
    HttpResponse response(404, "Not Found", "Key not found");

    EXPECT_EQ(response.getStatusCode(), 404);
    EXPECT_EQ(response.getStatusText(), "Not Found");
    EXPECT_EQ(response.getBody(), "Key not found");
}

TEST(HttpResponseTest, HandlesEmptyBody) {
    HttpResponse response(200, "OK", "");

    EXPECT_EQ(response.getBody(), "");
}