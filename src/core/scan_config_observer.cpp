#include "core/scan_config_observer.hpp"
#include "poco_config_adapter.hpp"
#include "logging/logger.hpp"
#include <algorithm>

void ScanConfigObserver::onConfigUpdate(const ConfigUpdateEvent &event)
{
    // Check for scan interval changes
    if (hasScanIntervalChange(event))
    {
        try
        {
            auto &config_manager = PocoConfigAdapter::getInstance();
            int new_interval = config_manager.getScanIntervalSeconds();
            handleScanIntervalChange(new_interval);
        }
        catch (const std::exception &e)
        {
            Logger::error("Error handling scan interval configuration change: " + std::string(e.what()));
        }
    }

    // Check for scan thread count changes
    if (hasScanThreadCountChange(event))
    {
        try
        {
            auto &config_manager = PocoConfigAdapter::getInstance();
            int new_thread_count = config_manager.getMaxScanThreads();
            handleScanThreadCountChange(new_thread_count);
        }
        catch (const std::exception &e)
        {
            Logger::error("Error handling scan thread count configuration change: " + std::string(e.what()));
        }
    }
}

bool ScanConfigObserver::hasScanIntervalChange(const ConfigUpdateEvent &event) const
{
    return std::find(event.changed_keys.begin(), event.changed_keys.end(), "scan_interval_seconds") != event.changed_keys.end();
}

bool ScanConfigObserver::hasScanThreadCountChange(const ConfigUpdateEvent &event) const
{
    return std::find(event.changed_keys.begin(), event.changed_keys.end(), "max_scan_threads") != event.changed_keys.end();
}

void ScanConfigObserver::handleScanIntervalChange(int new_interval)
{
    Logger::info("ScanConfigObserver: Scan interval configuration changed to: " + std::to_string(new_interval) + " seconds");

    // TODO: Implement actual scan interval change logic
    // This would involve:
    // 1. Stopping the current scan scheduler
    // 2. Reconfiguring the scheduler with the new interval
    // 3. Restarting the scheduler with the new interval
    // 4. Logging the change for audit purposes

    Logger::warn("ScanConfigObserver: Scan interval change detected but not yet implemented. "
                 "Scheduler restart required to apply new interval: " +
                 std::to_string(new_interval) + " seconds");
}

void ScanConfigObserver::handleScanThreadCountChange(int new_thread_count)
{
    Logger::info("ScanConfigObserver: Scan thread count configuration changed to: " + std::to_string(new_thread_count));

    // TODO: Implement actual scan thread count change logic
    // This would involve:
    // 1. Updating the scan thread pool configuration
    // 2. Restarting scan operations with the new thread count
    // 3. Logging the change for audit purposes

    Logger::warn("ScanConfigObserver: Scan thread count change detected but not yet implemented. "
                 "Scan operations restart required to apply new thread count: " +
                 std::to_string(new_thread_count));
}
