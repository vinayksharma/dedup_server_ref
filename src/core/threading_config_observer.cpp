#include "core/threading_config_observer.hpp"
#include "poco_config_adapter.hpp"
#include "core/thread_pool_manager.hpp"
#include "logging/logger.hpp"
#include <algorithm>

void ThreadingConfigObserver::onConfigUpdate(const ConfigUpdateEvent &event)
{
    // Check for max processing threads changes
    if (hasMaxProcessingThreadsChange(event))
    {
        try
        {
            auto &config_manager = PocoConfigAdapter::getInstance();
            int new_thread_count = config_manager.getMaxProcessingThreads();
            handleMaxProcessingThreadsChange(new_thread_count);
        }
        catch (const std::exception &e)
        {
            Logger::error("Error handling max processing threads configuration change: " + std::string(e.what()));
        }
    }

    // Check for max scan threads changes
    if (hasMaxScanThreadsChange(event))
    {
        try
        {
            auto &config_manager = PocoConfigAdapter::getInstance();
            int new_thread_count = config_manager.getMaxScanThreads();
            handleMaxScanThreadsChange(new_thread_count);
        }
        catch (const std::exception &e)
        {
            Logger::error("Error handling max scan threads configuration change: " + std::string(e.what()));
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

    // Check for HTTP server threads changes
    if (hasHttpServerThreadsChange(event))
    {
        try
        {
            auto &config_manager = PocoConfigAdapter::getInstance();
            std::string new_threads = config_manager.getHttpServerThreads();
            handleHttpServerThreadsChange(new_threads);
        }
        catch (const std::exception &e)
        {
            Logger::error("Error handling HTTP server threads configuration change: " + std::string(e.what()));
        }
    }
}

bool ThreadingConfigObserver::hasMaxProcessingThreadsChange(const ConfigUpdateEvent &event) const
{
    return std::find(event.changed_keys.begin(), event.changed_keys.end(), "max_processing_threads") != event.changed_keys.end();
}

bool ThreadingConfigObserver::hasMaxScanThreadsChange(const ConfigUpdateEvent &event) const
{
    return std::find(event.changed_keys.begin(), event.changed_keys.end(), "max_scan_threads") != event.changed_keys.end();
}

bool ThreadingConfigObserver::hasDatabaseThreadsChange(const ConfigUpdateEvent &event) const
{
    return std::find(event.changed_keys.begin(), event.changed_keys.end(), "database_threads") != event.changed_keys.end();
}

bool ThreadingConfigObserver::hasHttpServerThreadsChange(const ConfigUpdateEvent &event) const
{
    return std::find(event.changed_keys.begin(), event.changed_keys.end(), "http_server_threads") != event.changed_keys.end();
}

void ThreadingConfigObserver::handleMaxProcessingThreadsChange(int new_thread_count)
{
    Logger::info("ThreadingConfigObserver: Max processing threads configuration changed to: " + std::to_string(new_thread_count));

    // TODO: In a real implementation, this would call ThreadPoolManager::resizeThreadPool()
    // For now, just log the change to avoid linking issues in tests
    Logger::info("ThreadingConfigObserver: Would resize thread pool to " + std::to_string(new_thread_count) + " threads");
}

void ThreadingConfigObserver::handleMaxScanThreadsChange(int new_thread_count)
{
    Logger::info("ThreadingConfigObserver: Max scan threads configuration changed to: " + std::to_string(new_thread_count));

    // TODO: Implement actual scan thread count change logic
    // This would involve:
    // 1. Updating the scan thread pool configuration
    // 2. Restarting scan operations with the new thread count
    // 3. Logging the change for audit purposes

    Logger::warn("ThreadingConfigObserver: Max scan threads change detected but not yet implemented. "
                 "Scan operations restart required to apply new thread count: " +
                 std::to_string(new_thread_count));
}

void ThreadingConfigObserver::handleDatabaseThreadsChange(int new_thread_count)
{
    Logger::info("ThreadingConfigObserver: Database threads configuration changed to: " + std::to_string(new_thread_count));

    // TODO: Implement actual database thread count change logic
    // This would involve:
    // 1. Updating the database connection pool configuration
    // 2. Restarting database operations with the new thread count
    // 3. Logging the change for audit purposes

    Logger::warn("ThreadingConfigObserver: Database threads change detected but not yet implemented. "
                 "Database operations restart required to apply new thread count: " +
                 std::to_string(new_thread_count));
}

void ThreadingConfigObserver::handleHttpServerThreadsChange(const std::string &new_threads)
{
    Logger::info("ThreadingConfigObserver: HTTP server threads configuration changed to: " + new_threads);

    // TODO: Implement actual HTTP server threads change logic
    // This would involve:
    // 1. Updating the HTTP server thread pool configuration
    // 2. Restarting the HTTP server with the new thread count
    // 3. Logging the change for audit purposes

    Logger::warn("ThreadingConfigObserver: HTTP server threads change detected but not yet implemented. "
                 "Server restart required to apply new thread configuration: " +
                 new_threads);
}
