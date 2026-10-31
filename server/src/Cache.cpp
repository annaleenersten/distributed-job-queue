#include "Cache.h"

Cache::Cache(std::size_t maxSize)
    : maxCacheSize(maxSize) {
}

std::size_t Cache::getHits() const{
    return hits;
}

std::size_t Cache::getMisses() const{
    return misses;
}

void Cache::set(
    const std::string& key,
    const std::string& value,
    std::chrono::milliseconds ttl
) {
    std::lock_guard<std::mutex> lock(cacheMutex);

    CacheEntry entry;
    entry.value = value;
    entry.expiresAt = std::chrono::steady_clock::now() + ttl;

    auto existing = cache.find(key);

    if (existing != cache.end()) {
        lruOrder.erase(existing->second.second);
        cache.erase(existing);
    }

    if (cache.size() >= maxCacheSize) {
        std::string lruKey = lruOrder.back();

        lruOrder.pop_back();
        cache.erase(lruKey);
    }

    lruOrder.push_front(key);

    cache[key] = {
        entry,
        lruOrder.begin()
    };
}

std::optional<std::string> Cache::get(
    const std::string& key
) {
    std::lock_guard<std::mutex> lock(cacheMutex);

    auto it = cache.find(key);

    if (it == cache.end()) {
        misses++;
        return std::nullopt;
    }

    if (std::chrono::steady_clock::now() >=
        it->second.first.expiresAt) {

        lruOrder.erase(it->second.second);
        cache.erase(it);

        misses++;
        return std::nullopt;
    }

    lruOrder.erase(it->second.second);
    lruOrder.push_front(key);

    it->second.second = lruOrder.begin();

    hits++;
    return it->second.first.value;
}

bool Cache::remove(const std::string& key) {
    std::lock_guard<std::mutex> lock(cacheMutex);

    auto it = cache.find(key);

    if (it == cache.end()) {
        return false;
    }

    lruOrder.erase(it->second.second);
    cache.erase(it);

    return true;
}

