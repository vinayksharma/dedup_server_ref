#pragma once

#include "config_observer.hpp"
#include "logging/logger.hpp"

/**
 * @brief Observer for processing-related configuration changes
 *
 * This observer reacts to changes in processing configurations
 * such as batch size, quality stack preprocessing, etc.
 */
class ProcessingConfigObserver : public ConfigObserver
{
public:
    ProcessingConfigObserver() = default;
    ~ProcessingConfigObserver() override = default;

    /**
     * @brief Handle configuration changes
     * @param event Configuration update event
     */
    void onConfigUpdate(const ConfigUpdateEvent &event) override;

private:
    /**
     * @brief Check if the event contains processing batch size changes
     * @param event Configuration update event
     * @return true if processing batch size changed
     */
    bool hasProcessingBatchSizeChange(const ConfigUpdateEvent &event) const;

    /**
     * @brief Check if the event contains quality stack preprocessing changes
     * @param event Configuration update event
     * @return true if quality stack preprocessing changed
     */
    bool hasQualityStackPreprocessingChange(const ConfigUpdateEvent &event) const;

    /**
     * @brief Handle processing batch size configuration change
     * @param new_batch_size New processing batch size
     */
    void handleProcessingBatchSizeChange(int new_batch_size);

    /**
     * @brief Handle quality stack preprocessing configuration change
     * @param enabled Whether quality stack preprocessing is enabled
     */
    void handleQualityStackPreprocessingChange(bool enabled);
};
