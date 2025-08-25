#include "core/logger_observer.hpp"
#include "poco_config_adapter.hpp"
#include "logging/logger.hpp"
#include <algorithm>

void LoggerObserver::onConfigUpdate(const ConfigUpdateEvent &event)
{
    if (hasLogLevelChange(event))
    {
        try
        {
            // Get the new log level from the configuration manager
            auto &config_manager = PocoConfigAdapter::getInstance();
            std::string new_log_level = config_manager.getLogLevel();

            Logger::info("LoggerObserver: Log level configuration change detected to: " + new_log_level);

            // Apply the new log level
            Logger::setLevel(new_log_level);

            Logger::info("LoggerObserver: Successfully updated log level to: " + new_log_level);
        }
        catch (const std::exception &e)
        {
            Logger::error("LoggerObserver: Error updating log level: " + std::string(e.what()));
        }
    }
}

bool LoggerObserver::hasLogLevelChange(const ConfigUpdateEvent &event) const
{
    return std::find(event.changed_keys.begin(), event.changed_keys.end(), "log_level") != event.changed_keys.end();
}
