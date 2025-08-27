#pragma once

#include "config_observer.hpp"
#include "logging/logger.hpp"

/**
 * @brief Observer for cache-related configuration changes
 *
 * This observer reacts to changes in cache configurations
 * such as decoder cache size, etc.
 */
class CacheConfigObserver : public ConfigObserver
{
public:
    CacheConfigObserver() = default;
    ~CacheConfigObserver() override = default;

    /**
     * @brief Handle configuration changes
     * @param event Configuration update event
     */
    void onConfigUpdate(const ConfigUpdateEvent &event) override;

private:
    /**
     * @brief Check if the event contains decoder cache size changes
     * @param event Configuration update event
     * @return true if decoder cache size changed
     */
    bool hasDecoderCacheSizeChange(const ConfigUpdateEvent &event) const;

    /**
     * @brief Handle decoder cache size configuration change
     * @param new_size_mb New decoder cache size in MB
     */
    void handleDecoderCacheSizeChange(uint32_t new_size_mb);
};
