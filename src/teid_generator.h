#pragma once

#include <mutex>
#include <random>
#include <unordered_set>

class TeidGenerator {
public:
    static uint32_t generate() {
        uint32_t teid;
        std::mt19937 gen{std::random_device{}()};
        std::uniform_int_distribution<uint32_t> dist(1, UINT32_MAX);

        while (true) {
            teid = dist(gen);
            {
                std::unique_lock lock(mutex_);
                if (used_teids_.insert(teid).second) {
                    return teid;
                }
            }
        }
    }

    static void release(uint32_t teid) {
        std::lock_guard lock(mutex_);
        used_teids_.erase(teid);
    }

private:
    static inline std::unordered_set<uint32_t> used_teids_;
    static inline std::mutex mutex_;
};
