#pragma once

#include "config_observer.hpp"
#include "logging/logger.hpp"

/**
 * @brief Observer for scan-related configuration changes
 *
 * This observer reacts to changes in scan-related configurations
 * such as scan interval, scan thread count, and other scanning settings.
 */
class ScanConfigObserver : public ConfigObserver
{
public:
    ScanConfigObserver() = default;
    ~ScanConfigObserver() override = default;

    /**
     * @brief Handle configuration changes
     * @param event Configuration update event
     */
    void onConfigUpdate(const ConfigUpdateEvent &event) override;

private:
    /**
     * @brief Check if the event contains scan interval changes
     * @param event Configuration update event
     * @return true if scan interval changed
     */
    bool hasScanIntervalChange(const ConfigUpdateEvent &event) const;

    /**
     * @brief Check if the event contains scan thread count changes
     * @param event Configuration update event
     * @return true if scan thread count changed
     */
    bool hasScanThreadCountChange(const ConfigUpdateEvent &event) const;

    /**
     * @brief Handle scan interval configuration change
     * @param new_interval New scan interval value in seconds
     */
    void handleScanIntervalChange(int new_interval);

    /**
     * @brief Handle scan thread count configuration change
     * @param new_thread_count New scan thread count value
     */
    void handleScanThreadCountChange(int new_thread_count);
};
