#pragma once

#include "config_observer.hpp"
#include "logging/logger.hpp"

/**
 * @brief Observer for video processing-related configuration changes
 *
 * This observer reacts to changes in video processing configurations
 * such as quality settings, frame skip settings, etc.
 */
class VideoProcessingConfigObserver : public ConfigObserver
{
public:
    VideoProcessingConfigObserver() = default;
    ~VideoProcessingConfigObserver() override = default;

    /**
     * @brief Handle configuration changes
     * @param event Configuration update event
     */
    void onConfigUpdate(const ConfigUpdateEvent &event) override;

private:
    /**
     * @brief Check if the event contains video processing quality changes
     * @param event Configuration update event
     * @return true if video processing quality settings changed
     */
    bool hasVideoProcessingQualityChange(const ConfigUpdateEvent &event) const;

    /**
     * @brief Check if the event contains video processing mode changes
     * @param event Configuration update event
     * @return true if video processing mode settings changed
     */
    bool hasVideoProcessingModeChange(const ConfigUpdateEvent &event) const;

    /**
     * @brief Handle video processing quality configuration change
     */
    void handleVideoProcessingQualityChange();

    /**
     * @brief Handle video processing mode configuration change
     */
    void handleVideoProcessingModeChange();
};
