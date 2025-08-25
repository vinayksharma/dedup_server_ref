#pragma once

#include "config_observer.hpp"
#include "logging/logger.hpp"

/**
 * @brief Observer that handles log level configuration changes
 */
class LoggerObserver : public ConfigObserver
{
public:
    LoggerObserver() = default;
    ~LoggerObserver() override = default;

    /**
     * @brief Handle configuration updates
     * @param event Configuration update event
     */
    void onConfigUpdate(const ConfigUpdateEvent &event) override;

private:
    /**
     * @brief Check if the event contains log level changes
     * @param event Configuration update event
     * @return True if log level was changed
     */
    bool hasLogLevelChange(const ConfigUpdateEvent &event) const;
};
