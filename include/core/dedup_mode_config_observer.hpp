#pragma once

#include "config_observer.hpp"
#include "logging/logger.hpp"
#include "core/dedup_modes.hpp"

/**
 * @brief Observer for dedup mode configuration changes
 *
 * This observer reacts to changes in dedup mode configuration
 * such as the overall dedup mode setting.
 */
class DedupModeConfigObserver : public ConfigObserver
{
public:
    DedupModeConfigObserver() = default;
    ~DedupModeConfigObserver() override = default;

    /**
     * @brief Handle configuration changes
     * @param event Configuration update event
     */
    void onConfigUpdate(const ConfigUpdateEvent &event) override;

private:
    /**
     * @brief Check if the event contains dedup mode changes
     * @param event Configuration update event
     * @return true if dedup mode changed
     */
    bool hasDedupModeChange(const ConfigUpdateEvent &event) const;

    /**
     * @brief Handle dedup mode configuration change
     * @param new_mode New dedup mode
     */
    void handleDedupModeChange(DedupMode new_mode);
};
