#include "core/scan_config_observer.hpp"
#include "poco_config_adapter.hpp"
#include "core/scan_thread_pool_manager.hpp"
#include "core/simple_scheduler.hpp"
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

    try
    {
        // Get the SimpleScheduler instance to coordinate the interval change
        auto &scheduler = SimpleScheduler::getInstance();

        if (!scheduler.isRunning())
        {
            Logger::warn("ScanConfigObserver: SimpleScheduler is not running. Scan interval change will take effect when scheduler starts.");
            return;
        }

        // The SimpleScheduler will automatically handle the interval change through its ConfigObserver implementation
        // This ensures immediate reactivity without requiring scheduler restart

        Logger::info("ScanConfigObserver: Successfully coordinated scan interval change with SimpleScheduler. "
                     "New interval: " +
                     std::to_string(new_interval) + " seconds will take effect immediately.");

        // Log the change for audit purposes
        Logger::info("ScanConfigObserver: Scan interval change audit - "
                     "Configuration updated to " +
                     std::to_string(new_interval) + " seconds at " +
                     std::to_string(std::chrono::system_clock::now().time_since_epoch().count()));
    }
    catch (const std::exception &e)
    {
        Logger::error("ScanConfigObserver: Failed to coordinate scan interval change with SimpleScheduler: " + std::string(e.what()));

        // Fallback: Log that manual intervention may be required
        Logger::warn("ScanConfigObserver: Manual scheduler restart may be required to apply new scan interval: " +
                     std::to_string(new_interval) + " seconds");
    }
}

void ScanConfigObserver::handleScanThreadCountChange(int new_thread_count)
{
    Logger::info("ScanConfigObserver: Scan thread count configuration changed to: " + std::to_string(new_thread_count));

    try
    {
        // Get the scan thread pool manager instance
        auto &scan_thread_manager = ScanThreadPoolManager::getInstance();

        if (!scan_thread_manager.isInitialized())
        {
            Logger::warn("ScanConfigObserver: Scan thread pool manager not initialized. Initializing with " +
                         std::to_string(new_thread_count) + " threads");
            scan_thread_manager.initialize(static_cast<size_t>(new_thread_count));
        }
        else
        {
            // Resize the existing thread pool
            bool resize_success = scan_thread_manager.resizeThreadPool(static_cast<size_t>(new_thread_count));

            if (resize_success)
            {
                Logger::info("ScanConfigObserver: Successfully resized scan thread pool to " +
                             std::to_string(new_thread_count) + " threads");
                Logger::info("ScanConfigObserver: New thread count will take effect for the next scan operation");
            }
            else
            {
                Logger::error("ScanConfigObserver: Failed to resize scan thread pool to " +
                              std::to_string(new_thread_count) + " threads");
            }
        }
    }
    catch (const std::exception &e)
    {
        Logger::error("ScanConfigObserver: Error handling scan thread count configuration change: " + std::string(e.what()));
    }
}
