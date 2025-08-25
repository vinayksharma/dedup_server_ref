#include "core/database_config_observer.hpp"
#include "poco_config_adapter.hpp"
#include "logging/logger.hpp"
#include <algorithm>

void DatabaseConfigObserver::onConfigUpdate(const ConfigUpdateEvent &event)
{
    // Check for database retry changes
    if (hasDatabaseRetryChange(event))
    {
        try
        {
            handleDatabaseRetryChange();
        }
        catch (const std::exception &e)
        {
            Logger::error("Error handling database retry configuration change: " + std::string(e.what()));
        }
    }

    // Check for database timeout changes
    if (hasDatabaseTimeoutChange(event))
    {
        try
        {
            handleDatabaseTimeoutChange();
        }
        catch (const std::exception &e)
        {
            Logger::error("Error handling database timeout configuration change: " + std::string(e.what()));
        }
    }
}

bool DatabaseConfigObserver::hasDatabaseRetryChange(const ConfigUpdateEvent &event) const
{
    return std::find(event.changed_keys.begin(), event.changed_keys.end(), "database.retry.max_attempts") != event.changed_keys.end() ||
           std::find(event.changed_keys.begin(), event.changed_keys.end(), "database.retry.backoff_base_ms") != event.changed_keys.end() ||
           std::find(event.changed_keys.begin(), event.changed_keys.end(), "database.retry.max_backoff_ms") != event.changed_keys.end();
}

bool DatabaseConfigObserver::hasDatabaseTimeoutChange(const ConfigUpdateEvent &event) const
{
    return std::find(event.changed_keys.begin(), event.changed_keys.end(), "database.timeout.busy_timeout_ms") != event.changed_keys.end() ||
           std::find(event.changed_keys.begin(), event.changed_keys.end(), "database.timeout.operation_timeout_ms") != event.changed_keys.end();
}

void DatabaseConfigObserver::handleDatabaseRetryChange()
{
    try
    {
        auto &config_manager = PocoConfigAdapter::getInstance();
        int max_attempts = config_manager.getDatabaseMaxRetries();
        int backoff_base_ms = config_manager.getDatabaseBackoffBaseMs();
        int max_backoff_ms = config_manager.getDatabaseMaxBackoffMs();

        Logger::info("DatabaseConfigObserver: Database retry configuration changed - "
                     "Max attempts: " +
                     std::to_string(max_attempts) +
                     ", Backoff base: " + std::to_string(backoff_base_ms) + "ms" +
                     ", Max backoff: " + std::to_string(max_backoff_ms) + "ms");

        // TODO: Implement actual database retry configuration change logic
        // This would involve:
        // 1. Updating the DatabaseManager's retry configuration
        // 2. Applying the new retry settings to existing database operations
        // 3. Logging the change for audit purposes

        Logger::warn("DatabaseConfigObserver: Database retry change detected but not yet implemented. "
                     "Database restart required to apply new retry settings.");
    }
    catch (const std::exception &e)
    {
        Logger::error("DatabaseConfigObserver: Error getting database retry configuration: " + std::string(e.what()));
    }
}

void DatabaseConfigObserver::handleDatabaseTimeoutChange()
{
    try
    {
        auto &config_manager = PocoConfigAdapter::getInstance();
        int busy_timeout_ms = config_manager.getDatabaseBusyTimeoutMs();
        int operation_timeout_ms = config_manager.getDatabaseOperationTimeoutMs();

        Logger::info("DatabaseConfigObserver: Database timeout configuration changed - "
                     "Busy timeout: " +
                     std::to_string(busy_timeout_ms) + "ms" +
                     ", Operation timeout: " + std::to_string(operation_timeout_ms) + "ms");

        // TODO: Implement actual database timeout configuration change logic
        // This would involve:
        // 1. Updating the DatabaseManager's timeout configuration
        // 2. Applying the new timeout settings to existing database operations
        // 3. Logging the change for audit purposes

        Logger::warn("DatabaseConfigObserver: Database timeout change detected but not yet implemented. "
                     "Database restart required to apply new timeout settings.");
    }
    catch (const std::exception &e)
    {
        Logger::error("DatabaseConfigObserver: Error getting database timeout configuration: " + std::string(e.what()));
    }
}
