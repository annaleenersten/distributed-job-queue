#include "Cache.h"

#include <gtest/gtest.h>

#include <chrono>
#include <thread>

TEST(CacheTest, StoresAndRetrievesValue) {
    Cache cache{1000};

    cache.set("name", "Annalee", std::chrono::seconds(10));

    ASSERT_TRUE(cache.get("name").has_value());
    EXPECT_EQ(cache.get("name").value(), "Annalee");
}

TEST(CacheTest, UpdatesExistingValue) {
    Cache cache{1000};

    cache.set("name", "Annalee", std::chrono::seconds(10));
    cache.set("name", "Test", std::chrono::seconds(10));

    ASSERT_TRUE(cache.get("name").has_value());
    EXPECT_EQ(cache.get("name").value(), "Test");
}

TEST(CacheTest, ReturnsEmptyForMissingKey) {
    Cache cache{1000};

    EXPECT_FALSE(cache.get("missing").has_value());
}

TEST(CacheTest, RemovesValue) {
    Cache cache{1000};

    cache.set("name", "Annalee", std::chrono::seconds(10));

    EXPECT_TRUE(cache.remove("name"));
    EXPECT_FALSE(cache.get("name").has_value());
}

TEST(CacheTest, ReturnsFalseWhenRemovingMissingKey) {
    Cache cache{1000};

    EXPECT_FALSE(cache.remove("missing"));
}

TEST(CacheTest, ValueExpiresAfterTTL) {
    Cache cache{1000};

    cache.set("name", "Annalee", std::chrono::milliseconds(50));

    ASSERT_TRUE(cache.get("name").has_value());

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    EXPECT_FALSE(cache.get("name").has_value());
}

TEST(CacheTest, EvictsLeastRecentlyUsedValue) {
    Cache cache(3);

    cache.set("A", "1", std::chrono::seconds(10));
    cache.set("B", "2", std::chrono::seconds(10));
    cache.set("C", "3", std::chrono::seconds(10));

    cache.get("A");

    cache.set("D", "4", std::chrono::seconds(10));

    EXPECT_TRUE(cache.get("A").has_value());
    EXPECT_FALSE(cache.get("B").has_value());
    EXPECT_TRUE(cache.get("C").has_value());
    EXPECT_TRUE(cache.get("D").has_value());
}

TEST(CacheTest, UpdatingValueDoesNotEvictAnotherValue) {
    Cache cache(3);

    cache.set("A", "1", std::chrono::seconds(10));
    cache.set("B", "2", std::chrono::seconds(10));
    cache.set("C", "3", std::chrono::seconds(10));

    cache.set("B", "updated", std::chrono::seconds(10));

    EXPECT_EQ(cache.get("B").value(), "updated");
    EXPECT_TRUE(cache.get("A").has_value());
    EXPECT_TRUE(cache.get("C").has_value());
}

TEST(CacheTest, GetUpdatesLRUOrder) {
    Cache cache(3);

    cache.set("A", "1", std::chrono::seconds(10));
    cache.set("B", "2", std::chrono::seconds(10));
    cache.set("C", "3", std::chrono::seconds(10));

    cache.get("A");
    cache.set("D", "4", std::chrono::seconds(10));

    EXPECT_TRUE(cache.get("A").has_value());
    EXPECT_FALSE(cache.get("B").has_value());
}

TEST(CacheTest, TracksHits) {
    Cache cache(3);

    cache.set("A", "1", std::chrono::seconds(10));

    cache.get("A");
    cache.get("A");

    EXPECT_EQ(cache.getHits(), 2);
}

TEST(CacheTest, TracksMisses) {
    Cache cache(3);

    cache.get("missing");

    EXPECT_EQ(cache.getMisses(), 1);
}

TEST(CacheTest, ExpiredValueCountsAsMiss) {
    Cache cache(3);

    cache.set("A", "1", std::chrono::milliseconds(50));

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    EXPECT_FALSE(cache.get("A").has_value());
    EXPECT_EQ(cache.getMisses(), 1);
}