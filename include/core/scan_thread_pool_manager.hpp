#pragma once

#include <memory>
#include <atomic>
#include <mutex>
#include <tbb/global_control.h>
#include "logging/logger.hpp"

/**
 * @brief Manages scan thread pools with dynamic resizing capability
 *
 * This class provides a centralized way to manage scan thread pools
 * and allows dynamic resizing based on configuration changes.
 */
class ScanThreadPoolManager
{
public:
    // Singleton pattern
    static ScanThreadPoolManager &getInstance();

    // Constructor (made public for std::make_unique)
    ScanThreadPoolManager();

    // Thread pool management
    bool initialize(size_t num_threads);
    bool resizeThreadPool(size_t new_num_threads);
    void shutdown();

    // Getters
    size_t getCurrentThreadCount() const { return current_thread_count_.load(); }
    bool isInitialized() const { return initialized_.load(); }

    // Configuration validation
    static bool validateThreadCount(size_t num_threads);

    // Destructor
    ~ScanThreadPoolManager();

private:
    // Disable copy and assignment
    ScanThreadPoolManager(const ScanThreadPoolManager &) = delete;
    ScanThreadPoolManager &operator=(const ScanThreadPoolManager &) = delete;

    // Member variables
    std::atomic<bool> initialized_{false};
    std::atomic<size_t> current_thread_count_{0};
    std::unique_ptr<tbb::global_control> global_control_;

    // Thread safety
    mutable std::mutex resize_mutex_;

    // Constants
    static constexpr size_t MIN_THREADS = 1;
    static constexpr size_t MAX_THREADS = 64;
    static constexpr size_t DEFAULT_THREADS = 4;
};
