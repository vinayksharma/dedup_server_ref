#include "core/logger_observer.hpp"
#include "logging/logger.hpp"
#include <algorithm>

void LoggerObserver::onConfigUpdate(const ConfigUpdateEvent &event)
{
    if (hasLogLevelChange(event))
    {
        // Get the new log level from the configuration manager
        // We'll need to access the config manager to get the current value
        // For now, we'll just log that we detected a log level change
        Logger::info("LoggerObserver: Log level configuration change detected");

        // TODO: Implement actual log level change logic
        // This would involve:
        // 1. Getting the new log level from the config manager
        // 2. Calling Logger::setLevel() with the new level
        // 3. Logging the change for audit purposes
    }
}

bool LoggerObserver::hasLogLevelChange(const ConfigUpdateEvent &event) const
{
    return std::find(event.changed_keys.begin(), event.changed_keys.end(), "log_level") != event.changed_keys.end();
}
