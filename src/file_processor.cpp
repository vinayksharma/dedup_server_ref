#include "core/file_processor.hpp"
#include "database/database_manager.hpp"
#include "core/media_processor.hpp"
#include "poco_config_adapter.hpp"
#include "logging/logger.hpp"
#include <iostream>
#include <chrono>
#include <algorithm>

FileProcessor::FileProcessor(const std::string &db_path)
    : total_files_processed_(0), successful_files_processed_(0),
      is_processing_active_(false), cache_clear_required_(false)
{
    db_manager_ = &DatabaseManager::getInstance();

    // Get initial dedup mode from configuration
    auto &config_manager = PocoConfigAdapter::getInstance();
    current_dedup_mode_ = config_manager.getDedupMode();

    Logger::info("FileProcessor initialized with database: " + db_path +
                 ", initial dedup mode: " + DedupModes::getModeName(current_dedup_mode_));

    // Subscribe to configuration changes only if not in TEST_MODE
    const char *test_mode = std::getenv("TEST_MODE");
    if (!test_mode || std::string(test_mode) != "1")
    {
        PocoConfigAdapter::getInstance().subscribe(this);
        Logger::debug("FileProcessor subscribed to configuration changes");
    }
    else
    {
        Logger::debug("FileProcessor: TEST_MODE detected, skipping configuration subscription");
    }
}

FileProcessor::~FileProcessor()
{
    if (db_manager_)
    {
        db_manager_->waitForWrites();
    }

    // Unsubscribe from configuration changes only if not in TEST_MODE
    const char *test_mode = std::getenv("TEST_MODE");
    if (!test_mode || std::string(test_mode) != "1")
    {
        PocoConfigAdapter::getInstance().unsubscribe(this);
        Logger::debug("FileProcessor unsubscribed from configuration changes");
    }
}

size_t FileProcessor::processDirectory(const std::string &dir_path, bool recursive)
{
    Logger::info("Starting directory processing: " + dir_path + " (recursive: " + (recursive ? "yes" : "no") + ")");

    // Clear previous stats
    clearStats();

    // Mark processing as active
    is_processing_active_.store(true);

    try
    {
        // Get current quality mode from configuration
        auto &config_manager = PocoConfigAdapter::getInstance();
        DedupMode current_mode = config_manager.getDedupMode();

        Logger::info("Using quality mode: " + DedupModes::getModeName(current_mode));

        // Subscribe to file stream
        auto file_stream = FileUtils::listFilesAsObservable(dir_path, recursive);

        file_stream.subscribe(
            [this](const std::string &file_path)
            {
                this->handleFile(file_path);
            },
            [this](const std::exception &error)
            {
                this->handleError(error);
            },
            [this]()
            {
                this->handleComplete();
            });

        Logger::info("Directory processing completed. Processed " +
                     std::to_string(total_files_processed_) + " files, " +
                     std::to_string(successful_files_processed_) + " successful");
    }
    catch (const std::exception &e)
    {
        Logger::error("Error during directory processing: " + std::string(e.what()));
    }

    // Mark processing as inactive
    is_processing_active_.store(false);

    // Apply any pending mode changes now that processing is complete
    applyPendingModeChanges();

    return total_files_processed_;
}

FileProcessResult FileProcessor::processFile(const std::string &file_path)
{
    Logger::info("Processing single file: " + file_path);

    // Mark processing as active for this file
    startProcessingFile(file_path);

    try
    {
        if (!MediaProcessor::isSupportedFile(file_path))
        {
            std::string msg = "Unsupported file type: " + file_path;
            Logger::warn(msg);
            finishProcessingFile(file_path);
            return FileProcessResult(false, msg);
        }

        // Check if file exists in scanned_files table before processing
        if (!db_manager_->fileExistsInDatabase(file_path))
        {
            std::string msg = "File not found in scanned_files table: " + file_path + ". File must be scanned before processing.";
            Logger::warn(msg);
            finishProcessingFile(file_path);
            return FileProcessResult(false, msg);
        }

        auto &config_manager = PocoConfigAdapter::getInstance();
        DedupMode current_mode = config_manager.getDedupMode();
        ProcessingResult result = MediaProcessor::processFile(file_path, current_mode);
        DBOpResult db_result = db_manager_->storeProcessingResult(file_path, current_mode, result);
        if (!db_result.success)
        {
            std::string msg = "Failed to store processing result for: " + file_path + ". DB error: " + db_result.error_message;
            Logger::error(msg);
            finishProcessingFile(file_path);
            return FileProcessResult(false, msg);
        }

        // Handle processing flag based on result
        if (result.success)
        {
            // Success: Set processing flag to 1 (completed)
            auto flag_result = db_manager_->setProcessingFlag(file_path, current_mode);
            if (!flag_result.success)
            {
                Logger::warn("Failed to set processing flag after successful processing: " + file_path + " - " + flag_result.error_message);
            }
            else
            {
                Logger::debug("Set processing flag to completed (1) for: " + file_path + " mode: " + DedupModes::getModeName(current_mode));
            }
        }
        else
        {
            // Failure: Set processing flag to 2 (error state) to indicate processing failure
            auto flag_result = db_manager_->setProcessingFlagError(file_path, current_mode);
            if (!flag_result.success)
            {
                Logger::warn("Failed to set processing flag to error state after failed processing: " + file_path + " - " + flag_result.error_message);
            }
            else
            {
                Logger::debug("Set processing flag to error state (2) after failed processing: " + file_path + " mode: " + DedupModes::getModeName(current_mode));
            }
        }

        total_files_processed_++;
        if (result.success)
        {
            successful_files_processed_++;
            Logger::info("Successfully processed: " + file_path);
            finishProcessingFile(file_path);
            return FileProcessResult(true);
        }
        else
        {
            std::string msg = "Failed to process: " + file_path + " - " + result.error_message;
            Logger::warn(msg);
            finishProcessingFile(file_path);
            return FileProcessResult(false, msg);
        }
    }
    catch (const std::exception &e)
    {
        std::string msg = "Error processing file " + file_path + ": " + std::string(e.what());
        Logger::error(msg);

        // Set processing flag to 2 (error state) when exception occurs
        try
        {
            auto &config_manager = PocoConfigAdapter::getInstance();
            DedupMode current_mode = config_manager.getDedupMode();
            auto flag_result = db_manager_->setProcessingFlagError(file_path, current_mode);
            if (!flag_result.success)
            {
                Logger::warn("Failed to set processing flag to error state after exception: " + file_path + " - " + flag_result.error_message);
            }
            else
            {
                Logger::debug("Set processing flag to error state (2) after exception for: " + file_path + " mode: " + DedupModes::getModeName(current_mode));
            }
        }
        catch (...)
        {
            Logger::error("Failed to reset processing flag after exception for: " + file_path);
        }

        finishProcessingFile(file_path);
        return FileProcessResult(false, msg);
    }
}

std::pair<size_t, size_t> FileProcessor::getProcessingStats() const
{
    return {total_files_processed_, successful_files_processed_};
}

void FileProcessor::clearStats()
{
    total_files_processed_ = 0;
    successful_files_processed_ = 0;
}

void FileProcessor::waitForWrites()
{
    if (db_manager_)
    {
        db_manager_->waitForWrites();
    }
}

std::string FileProcessor::getFileCategory(const std::string &file_path)
{
    // With the new configuration-driven approach, all supported file types
    // are treated equally. We return "Supported" for any file type that
    // is enabled in the configuration.
    if (MediaProcessor::isSupportedFile(file_path))
        return "Supported";
    return "Unknown";
}

bool FileProcessor::isProcessing() const
{
    return is_processing_active_.load();
}

DedupMode FileProcessor::getCurrentDedupMode() const
{
    std::lock_guard<std::mutex> lock(mode_change_mutex_);
    return current_dedup_mode_;
}

std::vector<PendingModeChange> FileProcessor::getPendingModeChanges() const
{
    std::lock_guard<std::mutex> lock(mode_change_mutex_);
    std::vector<PendingModeChange> result;

    // Copy the queue to a vector for safe return
    std::queue<PendingModeChange> temp_queue = pending_mode_changes_;
    while (!temp_queue.empty())
    {
        result.push_back(temp_queue.front());
        temp_queue.pop();
    }

    return result;
}

void FileProcessor::handleFile(const std::string &file_path)
{
    Logger::debug("Handling file: " + file_path);

    // Mark processing as active for this file
    startProcessingFile(file_path);

    try
    {
        if (!MediaProcessor::isSupportedFile(file_path))
        {
            Logger::debug("Skipping unsupported file: " + file_path);
            finishProcessingFile(file_path);
            return;
        }

        // Check if file exists in scanned_files table before processing
        if (!db_manager_->fileExistsInDatabase(file_path))
        {
            Logger::warn("File not found in scanned_files table: " + file_path + ". File must be scanned before processing.");
            finishProcessingFile(file_path);
            return;
        }

        auto &config_manager = PocoConfigAdapter::getInstance();
        DedupMode current_mode = config_manager.getDedupMode();
        ProcessingResult result = MediaProcessor::processFile(file_path, current_mode);
        DBOpResult db_result = db_manager_->storeProcessingResult(file_path, current_mode, result);
        if (!db_result.success)
        {
            Logger::error("Failed to store processing result for: " + file_path + ". DB error: " + db_result.error_message);
            finishProcessingFile(file_path);
            return;
        }

        // Handle processing flag based on result
        if (result.success)
        {
            // Success: Set processing flag to 1 (completed)
            auto flag_result = db_manager_->setProcessingFlag(file_path, current_mode);
            if (!flag_result.success)
            {
                Logger::warn("Failed to set processing flag after successful processing: " + file_path + " - " + flag_result.error_message);
            }
            else
            {
                Logger::debug("Set processing flag to completed (1) for: " + file_path + " mode: " + DedupModes::getModeName(current_mode));
            }
        }
        else
        {
            // Failure: Set processing flag to 2 (error state) to indicate processing failure
            auto flag_result = db_manager_->setProcessingFlagError(file_path, current_mode);
            if (!flag_result.success)
            {
                Logger::warn("Failed to set processing flag to error state after failed processing: " + file_path + " - " + flag_result.error_message);
            }
            else
            {
                Logger::debug("Set processing flag to error state (2) after failed processing: " + file_path + " mode: " + DedupModes::getModeName(current_mode));
            }
        }

        total_files_processed_++;
        if (result.success)
        {
            successful_files_processed_++;
            Logger::info("Successfully processed: " + file_path +
                         " (format: " + result.artifact.format +
                         ", confidence: " + std::to_string(result.artifact.confidence) + ")");
        }
        else
        {
            Logger::warn("Failed to process: " + file_path + " - " + result.error_message);
        }
    }
    catch (const std::exception &e)
    {
        Logger::error("Error handling file " + file_path + ": " + std::string(e.what()));

        // Set processing flag to 2 (error state) when exception occurs
        try
        {
            auto &config_manager = PocoConfigAdapter::getInstance();
            DedupMode current_mode = config_manager.getDedupMode();
            auto flag_result = db_manager_->setProcessingFlagError(file_path, current_mode);
            if (!flag_result.success)
            {
                Logger::warn("Failed to set processing flag to error state after exception: " + file_path + " - " + flag_result.error_message);
            }
            else
            {
                Logger::debug("Set processing flag to error state (2) after exception for: " + file_path + " mode: " + DedupModes::getModeName(current_mode));
            }
        }
        catch (...)
        {
            Logger::error("Failed to reset processing flag after exception for: " + file_path);
        }
    }

    // Mark processing as inactive for this file
    finishProcessingFile(file_path);
}

void FileProcessor::handleError(const std::exception &error)
{
    Logger::error("File processing error: " + std::string(error.what()));
}

void FileProcessor::handleComplete()
{
    Logger::info("File processing completed. Total: " + std::to_string(total_files_processed_) +
                 ", Successful: " + std::to_string(successful_files_processed_));
}

void FileProcessor::onConfigUpdate(const ConfigUpdateEvent &event)
{
    // Check if dedup_mode was changed
    for (const auto &key : event.changed_keys)
    {
        if (key == "dedup_mode")
        {
            auto &config_manager = PocoConfigAdapter::getInstance();
            DedupMode new_mode = config_manager.getDedupMode();

            Logger::info("FileProcessor: Deduplication mode change detected from " +
                         DedupModes::getModeName(current_dedup_mode_) + " to " +
                         DedupModes::getModeName(new_mode));

            // Queue the mode change for safe application
            queueModeChange(new_mode, "Configuration change via ConfigObserver");
            break;
        }
    }
}

bool FileProcessor::requiresCacheClearing(DedupMode old_mode, DedupMode new_mode) const
{
    // Cache clearing is required when switching between significantly different algorithms
    // FAST -> QUALITY or QUALITY -> FAST requires cache clearing due to different hash algorithms
    if ((old_mode == DedupMode::FAST && new_mode == DedupMode::QUALITY) ||
        (old_mode == DedupMode::QUALITY && new_mode == DedupMode::FAST))
    {
        return true;
    }

    // BALANCED mode changes may require cache clearing depending on the direction
    if (old_mode == DedupMode::BALANCED || new_mode == DedupMode::BALANCED)
    {
        // If moving to/from QUALITY mode, clear cache
        if (old_mode == DedupMode::QUALITY || new_mode == DedupMode::QUALITY)
        {
            return true;
        }
    }

    return false;
}

bool FileProcessor::isValidModeTransition(DedupMode old_mode, DedupMode new_mode) const
{
    // All mode transitions are valid, but some may require special handling
    if (old_mode == new_mode)
    {
        return false; // No change needed
    }

    // Validate that the new mode is a valid enum value
    switch (new_mode)
    {
    case DedupMode::FAST:
    case DedupMode::BALANCED:
    case DedupMode::QUALITY:
        return true;
    default:
        Logger::warn("FileProcessor: Invalid dedup mode transition to unknown mode");
        return false;
    }
}

void FileProcessor::queueModeChange(DedupMode new_mode, const std::string &reason)
{
    std::lock_guard<std::mutex> lock(mode_change_mutex_);

    if (!isValidModeTransition(current_dedup_mode_, new_mode))
    {
        Logger::warn("FileProcessor: Invalid mode transition from " +
                     DedupModes::getModeName(current_dedup_mode_) + " to " +
                     DedupModes::getModeName(new_mode) + " - ignoring change");
        return;
    }

    // Check if cache clearing is required
    if (requiresCacheClearing(current_dedup_mode_, new_mode))
    {
        std::lock_guard<std::mutex> cache_lock(cache_mutex_);
        cache_clear_required_ = true;
        Logger::info("FileProcessor: Cache clearing will be required for mode transition from " +
                     DedupModes::getModeName(current_dedup_mode_) + " to " +
                     DedupModes::getModeName(new_mode));
    }

    // Queue the mode change
    pending_mode_changes_.emplace(new_mode, reason);

    Logger::info("FileProcessor: Mode change queued from " +
                 DedupModes::getModeName(current_dedup_mode_) + " to " +
                 DedupModes::getModeName(new_mode) +
                 " (reason: " + reason + ")");
}

void FileProcessor::applyPendingModeChanges()
{
    std::lock_guard<std::mutex> lock(mode_change_mutex_);

    while (!pending_mode_changes_.empty())
    {
        PendingModeChange change = pending_mode_changes_.front();
        pending_mode_changes_.pop();

        DedupMode old_mode = current_dedup_mode_;
        current_dedup_mode_ = change.new_mode;

        // Log the mode change with audit trail
        logModeChange(old_mode, change.new_mode, change.reason);

        // Clear cache if required
        if (requiresCacheClearing(old_mode, change.new_mode))
        {
            clearDecoderCacheIfRequired();
        }

        Logger::info("FileProcessor: Successfully applied mode change from " +
                     DedupModes::getModeName(old_mode) + " to " +
                     DedupModes::getModeName(change.new_mode));
    }
}

void FileProcessor::clearDecoderCacheIfRequired()
{
    std::lock_guard<std::mutex> lock(cache_mutex_);

    if (cache_clear_required_)
    {
        Logger::info("FileProcessor: Clearing decoder cache due to dedup mode change");

        // TODO: Implement actual cache clearing logic
        // This would involve calling TranscodingManager's cache clearing methods
        // For now, we just log the requirement

        Logger::warn("FileProcessor: Cache clearing required but not yet implemented - transcoded files may need manual cleanup for optimal performance");

        cache_clear_required_ = false;
    }
}

void FileProcessor::logModeChange(DedupMode old_mode, DedupMode new_mode, const std::string &reason)
{
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);

    std::string timestamp = std::ctime(&time_t);
    timestamp.pop_back(); // Remove newline

    Logger::info("FileProcessor: DEDUP MODE CHANGE AUDIT TRAIL");
    Logger::info("  Timestamp: " + timestamp);
    Logger::info("  Previous Mode: " + DedupModes::getModeName(old_mode) +
                 " (" + DedupModes::getModeDescription(old_mode) + ")");
    Logger::info("  New Mode: " + DedupModes::getModeName(new_mode) +
                 " (" + DedupModes::getModeDescription(new_mode) + ")");
    Logger::info("  Reason: " + reason);
    Logger::info("  Library Stack: " + DedupModes::getLibraryStack(new_mode));
    Logger::info("  Cache Clearing Required: " +
                 std::string(requiresCacheClearing(old_mode, new_mode) ? "Yes" : "No"));
    Logger::info("  Processing State: " +
                 std::string(is_processing_active_.load() ? "Active" : "Inactive"));
}

void FileProcessor::startProcessingFile(const std::string &file_path)
{
    std::lock_guard<std::mutex> lock(processing_state_mutex_);
    currently_processing_files_.insert(file_path);
    Logger::debug("FileProcessor: Started processing file: " + file_path);
}

void FileProcessor::finishProcessingFile(const std::string &file_path)
{
    std::lock_guard<std::mutex> lock(processing_state_mutex_);
    auto it = currently_processing_files_.find(file_path);
    if (it != currently_processing_files_.end())
    {
        currently_processing_files_.erase(it);
        Logger::debug("FileProcessor: Finished processing file: " + file_path);
    }
}