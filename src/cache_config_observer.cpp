#include "core/cache_config_observer.hpp"
#include "poco_config_adapter.hpp"
#include "logging/logger.hpp"
#include <algorithm>

void CacheConfigObserver::onConfigUpdate(const ConfigUpdateEvent &event)
{
    if (hasDecoderCacheSizeChange(event))
    {
        auto &config = PocoConfigAdapter::getInstance();
        uint32_t new_size_mb = config.getDecoderCacheSizeMB();
        handleDecoderCacheSizeChange(new_size_mb);
    }
}

bool CacheConfigObserver::hasDecoderCacheSizeChange(const ConfigUpdateEvent &event) const
{
    return std::find(event.changed_keys.begin(), event.changed_keys.end(), "decoder_cache_size_mb") != event.changed_keys.end() ||
           std::find(event.changed_keys.begin(), event.changed_keys.end(), "cache.decoder_cache_size_mb") != event.changed_keys.end();
}

void CacheConfigObserver::handleDecoderCacheSizeChange(uint32_t new_size_mb)
{
    Logger::info("Cache configuration changed: decoder_cache_size_mb = " + std::to_string(new_size_mb) + " MB");

    // TODO: Implement cache size adjustment logic
    // This would typically involve:
    // 1. Notifying the decoder cache manager
    // 2. Adjusting cache limits
    // 3. Potentially clearing old entries if size is reduced

    Logger::info("Decoder cache size updated to " + std::to_string(new_size_mb) + " MB");
    Logger::warn("Note: Cache size changes may require cache cleanup and may affect performance temporarily");
}
