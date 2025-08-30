#include "core/decoder/media_decoder.hpp"
#include "poco_config_adapter.hpp"
#include "core/cache/decoder_cache.hpp"
#include "logging/logger.hpp"

MediaDecoder::MediaDecoder()
{
    // Get configuration from PocoConfigAdapter
    auto &config_manager = PocoConfigAdapter::getInstance();
    max_decoder_threads_ = config_manager.getMaxDecoderThreads();

    // Get cache size from DecoderCache
    auto &decoder_cache = DecoderCache::getInstance();
    cache_size_mb_ = decoder_cache.getCacheSizeMB();

    Logger::info("MediaDecoder initialized with " + std::to_string(max_decoder_threads_) +
                 " decoder threads and " + std::to_string(cache_size_mb_) + " MB cache");
}

MediaDecoder &MediaDecoder::getInstance()
{
    static MediaDecoder instance;
    return instance;
}

int MediaDecoder::getMaxDecoderThreads() const
{
    return max_decoder_threads_;
}

uint32_t MediaDecoder::getCacheSizeMB() const
{
    return cache_size_mb_;
}

void MediaDecoder::onConfigUpdate(const ConfigUpdateEvent &event)
{
    Logger::debug("MediaDecoder::onConfigUpdate called with " + std::to_string(event.changed_keys.size()) + " changed keys");

    // Check if max_decoder_threads was changed
    for (const auto &key : event.changed_keys)
    {
        Logger::debug("MediaDecoder: Checking key: " + key);

        if (key == "max_decoder_threads" || key == "threading.max_decoder_threads")
        {
            Logger::debug("MediaDecoder: max_decoder_threads key found in event");

            auto &config_manager = PocoConfigAdapter::getInstance();
            int new_max_threads = config_manager.getMaxDecoderThreads();

            Logger::debug("MediaDecoder: Current max_decoder_threads: " + std::to_string(max_decoder_threads_) +
                          ", New max_decoder_threads: " + std::to_string(new_max_threads));

            if (new_max_threads != max_decoder_threads_)
            {
                Logger::info("MediaDecoder: Updating max_decoder_threads from " +
                             std::to_string(max_decoder_threads_) + " to " +
                             std::to_string(new_max_threads));

                max_decoder_threads_ = new_max_threads;

                // Notify any components that depend on this value
                notifyMaxDecoderThreadsChanged(new_max_threads);
            }
            else
            {
                Logger::debug("MediaDecoder: max_decoder_threads value unchanged");
            }
            break;
        }
    }
}

void MediaDecoder::notifyMaxDecoderThreadsChanged(int new_max_threads)
{
    // This method is called when max_decoder_threads_ changes.
    // It can be used to notify other components that the maximum number of decoder threads
    // has been updated.
    Logger::debug("MediaDecoder: Notifying components that max_decoder_threads changed to " + std::to_string(new_max_threads));
}

void MediaDecoder::refreshConfiguration()
{
    // Refresh configuration from PocoConfigAdapter
    auto &config_manager = PocoConfigAdapter::getInstance();
    max_decoder_threads_ = config_manager.getMaxDecoderThreads();

    // Get cache size from DecoderCache
    auto &decoder_cache = DecoderCache::getInstance();
    cache_size_mb_ = decoder_cache.getCacheSizeMB();

    Logger::info("MediaDecoder: Configuration refreshed - " + std::to_string(max_decoder_threads_) +
                 " decoder threads and " + std::to_string(cache_size_mb_) + " MB cache");
}