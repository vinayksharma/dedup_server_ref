#include "core/dedup_mode_config_observer.hpp"
#include "poco_config_adapter.hpp"
#include "logging/logger.hpp"
#include <algorithm>

void DedupModeConfigObserver::onConfigUpdate(const ConfigUpdateEvent &event)
{
    if (hasDedupModeChange(event))
    {
        auto &config = PocoConfigAdapter::getInstance();
        DedupMode new_mode = config.getDedupMode();
        handleDedupModeChange(new_mode);
    }
}

bool DedupModeConfigObserver::hasDedupModeChange(const ConfigUpdateEvent &event) const
{
    return std::find(event.changed_keys.begin(), event.changed_keys.end(), "dedup_mode") != event.changed_keys.end();
}

void DedupModeConfigObserver::handleDedupModeChange(DedupMode new_mode)
{
    std::string mode_str;
    switch (new_mode)
    {
    case DedupMode::FAST:
        mode_str = "FAST";
        break;
    case DedupMode::BALANCED:
        mode_str = "BALANCED";
        break;
    case DedupMode::QUALITY:
        mode_str = "QUALITY";
        break;
    default:
        mode_str = "UNKNOWN";
        break;
    }

    Logger::info("Dedup mode configuration changed: dedup_mode = " + mode_str);

    // TODO: Implement dedup mode adjustment logic
    // This would typically involve:
    // 1. Notifying the duplicate linker
    // 2. Adjusting video processing parameters
    // 3. Updating deduplication algorithm settings
    // 4. Potentially clearing cached results that depend on the mode

    Logger::info("Dedup mode updated to " + mode_str);
    Logger::info("Note: Dedup mode changes will affect new deduplication operations");
    Logger::warn("Warning: Changing dedup mode may affect processing quality and performance");
}
