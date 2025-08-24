#pragma once

#include <string>

// Unified configuration change event types used across the codebase
enum class ConfigEventType
{
    DEDUP_MODE_CHANGED,
    LOG_LEVEL_CHANGED,
    SERVER_PORT_CHANGED,
    AUTH_SECRET_CHANGED,
    GENERAL_CONFIG_CHANGED
};

// Unified configuration change event payload
struct ConfigEvent
{
    ConfigEventType type;
    std::string key;
    std::string old_value;
    std::string new_value;
    std::string description;
};

// Observer interface for configuration changes
class ConfigObserver
{
public:
    virtual ~ConfigObserver() = default;
    virtual void onConfigChanged(const ConfigEvent &event) = 0;
};


