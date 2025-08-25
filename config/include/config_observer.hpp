#pragma once

#include <string>
#include <vector>

/**
 * @brief Configuration update event
 */
struct ConfigUpdateEvent
{
    std::vector<std::string> changed_keys; // Array of configuration keys that changed
    std::string source;                    // Source of the update: "api" or "file_observer"
    std::string update_id;                 // Unique identifier to prevent feedback loops
};

/**
 * @brief Observer interface for configuration changes
 */
class ConfigObserver
{
public:
    virtual ~ConfigObserver() = default;
    virtual void onConfigUpdate(const ConfigUpdateEvent &event) = 0;
};
