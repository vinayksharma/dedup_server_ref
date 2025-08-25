#include "core/video_processing_config_observer.hpp"
#include "poco_config_adapter.hpp"
#include "core/dedup_modes.hpp"
#include "logging/logger.hpp"
#include <algorithm>

void VideoProcessingConfigObserver::onConfigUpdate(const ConfigUpdateEvent &event)
{
    // Check for video processing quality changes
    if (hasVideoProcessingQualityChange(event))
    {
        try
        {
            handleVideoProcessingQualityChange();
        }
        catch (const std::exception &e)
        {
            Logger::error("Error handling video processing quality configuration change: " + std::string(e.what()));
        }
    }

    // Check for video processing mode changes
    if (hasVideoProcessingModeChange(event))
    {
        try
        {
            handleVideoProcessingModeChange();
        }
        catch (const std::exception &e)
        {
            Logger::error("Error handling video processing mode configuration change: " + std::string(e.what()));
        }
    }
}

bool VideoProcessingConfigObserver::hasVideoProcessingQualityChange(const ConfigUpdateEvent &event) const
{
    return std::find(event.changed_keys.begin(), event.changed_keys.end(), "video_processing.QUALITY") != event.changed_keys.end() ||
           std::find(event.changed_keys.begin(), event.changed_keys.end(), "video_processing.BALANCED") != event.changed_keys.end() ||
           std::find(event.changed_keys.begin(), event.changed_keys.end(), "video_processing.FAST") != event.changed_keys.end();
}

bool VideoProcessingConfigObserver::hasVideoProcessingModeChange(const ConfigUpdateEvent &event) const
{
    return std::find(event.changed_keys.begin(), event.changed_keys.end(), "dedup_mode") != event.changed_keys.end();
}

void VideoProcessingConfigObserver::handleVideoProcessingQualityChange()
{
    try
    {
        auto &config_manager = PocoConfigAdapter::getInstance();

        // Get current dedup mode
        auto current_mode = config_manager.getDedupMode();

        // Get quality settings for current mode
        int skip_duration = config_manager.getVideoSkipDurationSeconds(current_mode);
        int frames_per_skip = config_manager.getVideoFramesPerSkip(current_mode);
        int skip_count = config_manager.getVideoSkipCount(current_mode);

        Logger::info("VideoProcessingConfigObserver: Video processing quality configuration changed for mode: " +
                     DedupModes::getModeName(current_mode));
        Logger::info("VideoProcessingConfigObserver: Skip duration: " + std::to_string(skip_duration) + "s" +
                     ", Frames per skip: " + std::to_string(frames_per_skip) +
                     ", Skip count: " + std::to_string(skip_count));

        // TODO: Implement actual video processing quality configuration change logic
        // This would involve:
        // 1. Updating the TranscodingManager's video processing settings
        // 2. Refreshing any cached video processing configuration
        // 3. Logging the change for audit purposes

        Logger::warn("VideoProcessingConfigObserver: Video processing quality change detected but not yet implemented. "
                     "TranscodingManager restart required to apply new video processing settings.");
    }
    catch (const std::exception &e)
    {
        Logger::error("VideoProcessingConfigObserver: Error getting video processing quality configuration: " + std::string(e.what()));
    }
}

void VideoProcessingConfigObserver::handleVideoProcessingModeChange()
{
    try
    {
        auto &config_manager = PocoConfigAdapter::getInstance();
        auto new_mode = config_manager.getDedupMode();

        Logger::info("VideoProcessingConfigObserver: Video processing mode configuration changed to: " +
                     DedupModes::getModeName(new_mode));

        // Get quality settings for the new mode
        int skip_duration = config_manager.getVideoSkipDurationSeconds(new_mode);
        int frames_per_skip = config_manager.getVideoFramesPerSkip(new_mode);
        int skip_count = config_manager.getVideoSkipCount(new_mode);

        Logger::info("VideoProcessingConfigObserver: New mode settings - Skip duration: " + std::to_string(skip_duration) + "s" +
                     ", Frames per skip: " + std::to_string(frames_per_skip) +
                     ", Skip count: " + std::to_string(skip_count));

        // TODO: Implement actual video processing mode configuration change logic
        // This would involve:
        // 1. Updating the TranscodingManager's current processing mode
        // 2. Adjusting video processing parameters for the new mode
        // 3. Logging the change for audit purposes

        Logger::warn("VideoProcessingConfigObserver: Video processing mode change detected but not yet implemented. "
                     "TranscodingManager restart required to apply new processing mode.");
    }
    catch (const std::exception &e)
    {
        Logger::error("VideoProcessingConfigObserver: Error getting video processing mode configuration: " + std::string(e.what()));
    }
}
