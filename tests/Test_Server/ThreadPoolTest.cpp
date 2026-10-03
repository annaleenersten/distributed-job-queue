#include "ThreadPool.h"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <thread>

TEST(ThreadPoolTest, ExecutesTask) {
    ThreadPool pool(2);

    std::atomic<bool> completed = false;

    pool.enqueue([&completed]() {
        completed = true;
    });

    for (int i = 0; i < 100 && !completed; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_TRUE(completed);
}

TEST(ThreadPoolTest, ExecutesMultipleTasks) {
    ThreadPool pool(4);

    std::atomic<int> count = 0;

    for (int i = 0; i < 10; ++i) {
        pool.enqueue([&count]() {
            ++count;
        });
    }

    for (int i = 0; i < 100 && count < 10; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_EQ(count, 10);
}