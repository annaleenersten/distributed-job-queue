#pragma once

#include <string>
#include <unordered_map>
#include <mutex>
#include <optional>
#include <chrono>
#include <list>

struct CacheEntry {
    std::string value;
    std::chrono::steady_clock::time_point expiresAt;
};

class Cache {
public:
    explicit Cache(std::size_t maxSize);

    void set(
        const std::string& key,
        const std::string& value,
        std::chrono::milliseconds ttl
    );

    std::optional<std::string> get(const std::string& key);

    bool remove(const std::string& key);
    std::size_t getHits() const;
    std::size_t getMisses() const;

private:
    std::unordered_map<
        std::string,
        std::pair<CacheEntry, std::list<std::string>::iterator>
    > cache;

    std::list<std::string> lruOrder;

    std::mutex cacheMutex;
    std::size_t maxCacheSize;
    std::size_t hits = 0;
    std::size_t misses = 0;
};