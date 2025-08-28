#include "core/scan_thread_pool_manager.hpp"
#include "logging/logger.hpp"
#include <algorithm>

// Static instance pointer
static std::unique_ptr<ScanThreadPoolManager> instance_ptr;

ScanThreadPoolManager::ScanThreadPoolManager()
{
    Logger::info("ScanThreadPoolManager constructor called");
}

ScanThreadPoolManager::~ScanThreadPoolManager()
{
    shutdown();
    Logger::info("ScanThreadPoolManager destructor called");
}

ScanThreadPoolManager &ScanThreadPoolManager::getInstance()
{
    if (!instance_ptr)
    {
        instance_ptr = std::make_unique<ScanThreadPoolManager>();
    }
    return *instance_ptr;
}

bool ScanThreadPoolManager::initialize(size_t num_threads)
{
    if (initialized_.load())
    {
        Logger::warn("ScanThreadPoolManager already initialized");
        return true;
    }

    std::lock_guard<std::mutex> lock(resize_mutex_);
    if (initialized_.load()) // Double-check pattern
    {
        return true;
    }

    if (validateThreadCount(num_threads))
    {
        global_control_ = std::make_unique<tbb::global_control>(
            tbb::global_control::max_allowed_parallelism, num_threads);
        current_thread_count_.store(num_threads);
        initialized_.store(true);

        Logger::info("Scan thread pool manager initialized with " + std::to_string(num_threads) + " threads");
        return true;
    }
    else
    {
        Logger::error("Invalid scan thread count: " + std::to_string(num_threads) + ". Using default: " + std::to_string(DEFAULT_THREADS));

        global_control_ = std::make_unique<tbb::global_control>(
            tbb::global_control::max_allowed_parallelism, DEFAULT_THREADS);
        current_thread_count_.store(DEFAULT_THREADS);
        initialized_.store(true);

        Logger::info("Scan thread pool manager initialized with default " + std::to_string(DEFAULT_THREADS) + " threads");
        return true;
    }
}

bool ScanThreadPoolManager::resizeThreadPool(size_t new_num_threads)
{
    if (!initialized_.load())
    {
        Logger::error("Cannot resize scan thread pool: not initialized");
        return false;
    }

    if (!validateThreadCount(new_num_threads))
    {
        Logger::error("Invalid scan thread count for resize: " + std::to_string(new_num_threads));
        return false;
    }

    std::lock_guard<std::mutex> lock(resize_mutex_);

    size_t current_count = current_thread_count_.load();
    if (current_count == new_num_threads)
    {
        Logger::info("Scan thread pool already at requested size: " + std::to_string(new_num_threads));
        return true;
    }

    Logger::info("Resizing scan thread pool from " + std::to_string(current_count) + " to " + std::to_string(new_num_threads) + " threads");

    try
    {
        // Create new global control with new thread count
        global_control_ = std::make_unique<tbb::global_control>(
            tbb::global_control::max_allowed_parallelism, new_num_threads);

        current_thread_count_.store(new_num_threads);

        Logger::info("Successfully resized scan thread pool to " + std::to_string(new_num_threads) + " threads");
        return true;
    }
    catch (const std::exception &e)
    {
        Logger::error("Failed to resize scan thread pool: " + std::string(e.what()));
        return false;
    }
}

void ScanThreadPoolManager::shutdown()
{
    if (!initialized_.load())
    {
        return;
    }

    std::lock_guard<std::mutex> lock(resize_mutex_);

    if (global_control_)
    {
        global_control_.reset();
    }

    current_thread_count_.store(0);
    initialized_.store(false);

    Logger::info("Scan thread pool manager shutdown complete");
}

bool ScanThreadPoolManager::validateThreadCount(size_t num_threads)
{
    if (num_threads < MIN_THREADS || num_threads > MAX_THREADS)
    {
        Logger::warn("Scan thread count " + std::to_string(num_threads) + " is outside valid range [" +
                     std::to_string(MIN_THREADS) + ", " + std::to_string(MAX_THREADS) + "]");
        return false;
    }

    // Check if the number is reasonable for the system
    size_t hardware_concurrency = std::thread::hardware_concurrency();
    if (num_threads > hardware_concurrency * 2)
    {
        Logger::warn("Scan thread count " + std::to_string(num_threads) + " is significantly higher than hardware concurrency (" +
                     std::to_string(hardware_concurrency) + "). This may impact performance.");
    }

    return true;
}
