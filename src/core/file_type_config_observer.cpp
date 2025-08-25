#include "core/file_type_config_observer.hpp"
#include "poco_config_adapter.hpp"
#include "logging/logger.hpp"
#include <algorithm>

void FileTypeConfigObserver::onConfigUpdate(const ConfigUpdateEvent &event)
{
    // Check for file type category changes
    if (hasFileTypeCategoryChange(event))
    {
        try
        {
            handleFileTypeCategoryChange();
        }
        catch (const std::exception &e)
        {
            Logger::error("Error handling file type category configuration change: " + std::string(e.what()));
        }
    }

    // Check for transcoding file type changes
    if (hasTranscodingFileTypeChange(event))
    {
        try
        {
            handleTranscodingFileTypeChange();
        }
        catch (const std::exception &e)
        {
            Logger::error("Error handling transcoding file type configuration change: " + std::string(e.what()));
        }
    }
}

bool FileTypeConfigObserver::hasFileTypeCategoryChange(const ConfigUpdateEvent &event) const
{
    return std::find(event.changed_keys.begin(), event.changed_keys.end(), "categories.audio") != event.changed_keys.end() ||
           std::find(event.changed_keys.begin(), event.changed_keys.end(), "categories.images") != event.changed_keys.end() ||
           std::find(event.changed_keys.begin(), event.changed_keys.end(), "categories.images_raw") != event.changed_keys.end() ||
           std::find(event.changed_keys.begin(), event.changed_keys.end(), "categories.video") != event.changed_keys.end();
}

bool FileTypeConfigObserver::hasTranscodingFileTypeChange(const ConfigUpdateEvent &event) const
{
    return std::find(event.changed_keys.begin(), event.changed_keys.end(), "transcoding_file_types") != event.changed_keys.end();
}

void FileTypeConfigObserver::handleFileTypeCategoryChange()
{
    try
    {
        auto &config_manager = PocoConfigAdapter::getInstance();
        auto supported_types = config_manager.getSupportedFileTypes();
        auto enabled_types = config_manager.getEnabledFileTypes();

        Logger::info("FileTypeConfigObserver: File type category configuration changed");
        Logger::info("FileTypeConfigObserver: Total supported types: " + std::to_string(supported_types.size()));
        Logger::info("FileTypeConfigObserver: Total enabled types: " + std::to_string(enabled_types.size()));

        // Log some examples of enabled types
        if (!enabled_types.empty())
        {
            std::string examples;
            size_t count = 0;
            for (const auto &type : enabled_types)
            {
                if (count < 5) // Show first 5 types
                {
                    if (!examples.empty())
                        examples += ", ";
                    examples += type;
                    count++;
                }
                else
                {
                    break;
                }
            }
            if (count < enabled_types.size())
            {
                examples += " and " + std::to_string(enabled_types.size() - count) + " more";
            }
            Logger::info("FileTypeConfigObserver: Enabled types include: " + examples);
        }

        // TODO: Implement actual file type category configuration change logic
        // This would involve:
        // 1. Updating the MediaProcessor's supported file type list
        // 2. Refreshing any cached file type information
        // 3. Logging the change for audit purposes

        Logger::warn("FileTypeConfigObserver: File type category change detected but not yet implemented. "
                     "MediaProcessor restart required to apply new file type settings.");
    }
    catch (const std::exception &e)
    {
        Logger::error("FileTypeConfigObserver: Error getting file type configuration: " + std::string(e.what()));
    }
}

void FileTypeConfigObserver::handleTranscodingFileTypeChange()
{
    try
    {
        auto &config_manager = PocoConfigAdapter::getInstance();
        auto transcoding_types = config_manager.getTranscodingFileTypes();

        Logger::info("FileTypeConfigObserver: Transcoding file type configuration changed");
        Logger::info("FileTypeConfigObserver: Total transcoding types: " + std::to_string(transcoding_types.size()));

        // Log some examples of transcoding types
        if (!transcoding_types.empty())
        {
            std::string examples;
            size_t count = 0;
            for (const auto &[type, enabled] : transcoding_types)
            {
                if (enabled && count < 5) // Show first 5 enabled types
                {
                    if (!examples.empty())
                        examples += ", ";
                    examples += type;
                    count++;
                }
                else if (count >= 5)
                {
                    break;
                }
            }
            if (count < transcoding_types.size())
            {
                examples += " and " + std::to_string(transcoding_types.size() - count) + " more";
            }
            Logger::info("FileTypeConfigObserver: Transcoding types include: " + examples);
        }

        // TODO: Implement actual transcoding file type configuration change logic
        // This would involve:
        // 1. Updating the TranscodingManager's transcoding file type list
        // 2. Refreshing any cached transcoding information
        // 3. Logging the change for audit purposes

        Logger::warn("FileTypeConfigObserver: Transcoding file type change detected but not yet implemented. "
                     "TranscodingManager restart required to apply new transcoding settings.");
    }
    catch (const std::exception &e)
    {
        Logger::error("FileTypeConfigObserver: Error getting transcoding file type configuration: " + std::string(e.what()));
    }
}
