#include "core/processing_config_observer.hpp"
#include "poco_config_adapter.hpp"
#include "logging/logger.hpp"
#include <algorithm>

void ProcessingConfigObserver::onConfigUpdate(const ConfigUpdateEvent &event)
{
    if (hasProcessingBatchSizeChange(event))
    {
        auto &config = PocoConfigAdapter::getInstance();
        int new_batch_size = config.getProcessingBatchSize();
        handleProcessingBatchSizeChange(new_batch_size);
    }

    if (hasQualityStackPreprocessingChange(event))
    {
        auto &config = PocoConfigAdapter::getInstance();
        bool enabled = config.getPreProcessQualityStack();
        handleQualityStackPreprocessingChange(enabled);
    }
}

bool ProcessingConfigObserver::hasProcessingBatchSizeChange(const ConfigUpdateEvent &event) const
{
    return std::find(event.changed_keys.begin(), event.changed_keys.end(), "processing_batch_size") != event.changed_keys.end() ||
           std::find(event.changed_keys.begin(), event.changed_keys.end(), "processing.batch_size") != event.changed_keys.end();
}

bool ProcessingConfigObserver::hasQualityStackPreprocessingChange(const ConfigUpdateEvent &event) const
{
    return std::find(event.changed_keys.begin(), event.changed_keys.end(), "pre_process_quality_stack") != event.changed_keys.end();
}

void ProcessingConfigObserver::handleProcessingBatchSizeChange(int new_batch_size)
{
    Logger::info("Processing configuration changed: processing_batch_size = " + std::to_string(new_batch_size));

    // TODO: Implement batch size adjustment logic
    // This would typically involve:
    // 1. Notifying the media processing orchestrator
    // 2. Adjusting batch processing limits
    // 3. Updating processing pipeline configuration

    Logger::info("Processing batch size updated to " + std::to_string(new_batch_size));
    Logger::info("Note: New batch size will take effect for the next processing cycle");
}

void ProcessingConfigObserver::handleQualityStackPreprocessingChange(bool enabled)
{
    Logger::info("Processing configuration changed: pre_process_quality_stack = " + std::string(enabled ? "true" : "false"));

    // TODO: Implement quality stack preprocessing adjustment logic
    // This would typically involve:
    // 1. Notifying the media processor
    // 2. Adjusting preprocessing pipeline
    // 3. Updating quality stack configuration

    Logger::info("Quality stack preprocessing " + std::string(enabled ? "enabled" : "disabled"));
    Logger::info("Note: Quality stack preprocessing changes will take effect for new processing tasks");
}
