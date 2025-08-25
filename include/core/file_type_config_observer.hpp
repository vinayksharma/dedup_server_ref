#pragma once

#include "config_observer.hpp"
#include "logging/logger.hpp"

/**
 * @brief Observer for file type-related configuration changes
 *
 * This observer reacts to changes in file type configurations
 * such as supported file types, transcoding file types, etc.
 */
class FileTypeConfigObserver : public ConfigObserver
{
public:
    FileTypeConfigObserver() = default;
    ~FileTypeConfigObserver() override = default;

    /**
     * @brief Handle configuration changes
     * @param event Configuration update event
     */
    void onConfigUpdate(const ConfigUpdateEvent &event) override;

private:
    /**
     * @brief Check if the event contains file type category changes
     * @param event Configuration update event
     * @return true if file type category settings changed
     */
    bool hasFileTypeCategoryChange(const ConfigUpdateEvent &event) const;

    /**
     * @brief Check if the event contains transcoding file type changes
     * @param event Configuration update event
     * @return true if transcoding file type settings changed
     */
    bool hasTranscodingFileTypeChange(const ConfigUpdateEvent &event) const;

    /**
     * @brief Handle file type category configuration change
     */
    void handleFileTypeCategoryChange();

    /**
     * @brief Handle transcoding file type configuration change
     */
    void handleTranscodingFileTypeChange();
};
