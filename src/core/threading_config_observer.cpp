#include "core/threading_config_observer.hpp"
#include "poco_config_adapter.hpp"
#include "core/thread_pool_manager.hpp"
// #include "core/scan_thread_pool_manager.hpp"  // Removed - no longer needed
#include "core/database_connection_pool.hpp"
#include "logging/logger.hpp"
#include <algorithm>

void ThreadingConfigObserver::onConfigUpdate(const ConfigUpdateEvent &event)
{
    // Check for processing threads changes
    if (hasProcessingThreadsChange(event))
    {
        try
        {
            auto &config_manager = PocoConfigAdapter::getInstance();
            int new_thread_count = config_manager.getMaxProcessingThreads();
            handleProcessingThreadsChange(new_thread_count);
        }
        catch (const std::exception &e)
        {
            Logger::error("Error handling processing threads configuration change: " + std::string(e.what()));
        }
    }

    // Check for scan threads changes
    if (hasScanThreadsChange(event))
    {
        try
        {
            auto &config_manager = PocoConfigAdapter::getInstance();
            int new_thread_count = config_manager.getMaxScanThreads();
            handleScanThreadsChange(new_thread_count);
        }
        catch (const std::exception &e)
        {
            Logger::error("Error handling scan threads configuration change: " + std::string(e.what()));
        }
    }

    // Check for database threads changes
    if (hasDatabaseThreadsChange(event))
    {
        try
        {
            auto &config_manager = PocoConfigAdapter::getInstance();
            int new_thread_count = config_manager.getDatabaseThreads();
            handleDatabaseThreadsChange(new_thread_count);
        }
        catch (const std::exception &e)
        {
            Logger::error("Error handling database threads configuration change: " + std::string(e.what()));
        }
    }
}

bool ThreadingConfigObserver::hasProcessingThreadsChange(const ConfigUpdateEvent &event) const
{
    return std::find(event.changed_keys.begin(), event.changed_keys.end(), "max_processing_threads") != event.changed_keys.end();
}

bool ThreadingConfigObserver::hasScanThreadsChange(const ConfigUpdateEvent &event) const
{
    return std::find(event.changed_keys.begin(), event.changed_keys.end(), "max_scan_threads") != event.changed_keys.end();
}

bool ThreadingConfigObserver::hasDatabaseThreadsChange(const ConfigUpdateEvent &event) const
{
    return std::find(event.changed_keys.begin(), event.changed_keys.end(), "database_threads") != event.changed_keys.end();
}

void ThreadingConfigObserver::handleProcessingThreadsChange(int new_thread_count)
{
    Logger::info("ThreadingConfigObserver: Processing threads configuration changed to: " + std::to_string(new_thread_count));

    // Note: ThreadPoolManager replaced with ContinuousProcessingManager
    // Single thread architecture doesn't support resizing
    Logger::info("ThreadingConfigObserver: Processing thread count change ignored - single thread architecture");
    Logger::info("ThreadingConfigObserver: ContinuousProcessingManager uses single thread for processing");
}

void ThreadingConfigObserver::handleScanThreadsChange(int new_thread_count)
{
    Logger::info("ThreadingConfigObserver: Scan threads configuration changed to: " + std::to_string(new_thread_count));

    // Note: ScanThreadPoolManager removed - using simple sequential scanning
    Logger::info("ThreadingConfigObserver: Scan thread count change ignored - using sequential scanning");
    Logger::info("ThreadingConfigObserver: Sequential scanning doesn't support thread count changes");
}

void ThreadingConfigObserver::handleDatabaseThreadsChange(int new_thread_count)
{
    Logger::info("ThreadingConfigObserver: Database threads configuration changed to: " + std::to_string(new_thread_count));

    // Get the database connection pool instance
    auto &db_connection_pool = DatabaseConnectionPool::getInstance();

    if (!db_connection_pool.isInitialized())
    {
        Logger::warn("ThreadingConfigObserver: Database connection pool not initialized. Initializing with " +
                     std::to_string(new_thread_count) + " connections");
        db_connection_pool.initialize(static_cast<size_t>(new_thread_count));
    }
    else
    {
        // Resize the existing connection pool
        bool resize_success = db_connection_pool.resizeConnectionPool(static_cast<size_t>(new_thread_count));

        if (resize_success)
        {
            Logger::info("ThreadingConfigObserver: Successfully resized database connection pool to " +
                         std::to_string(new_thread_count) + " connections");
            Logger::info("ThreadingConfigObserver: New connection count will take effect for the next database operation");
        }
        else
        {
            Logger::error("ThreadingConfigObserver: Failed to resize database connection pool to " +
                          std::to_string(new_thread_count) + " connections");
        }
    }
}
