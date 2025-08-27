#pragma once

#include "database/database_manager.hpp"
#include <string>
#include <vector>
#include <functional>
#include <queue>
#include <mutex>
#include <atomic>
#include <unordered_set>
#include "file_utils.hpp"
#include "media_processor.hpp"
#include "poco_config_adapter.hpp"
#include "config_observer.hpp"
#include "core/dedup_modes.hpp"

/**
 * @brief Result of a file processing operation
 */
struct FileProcessResult
{
    bool success;
    std::string error_message;
    FileProcessResult(bool s = true, const std::string &msg = "") : success(s), error_message(msg) {}
};

/**
 * @brief Pending mode change request
 */
struct PendingModeChange
{
    DedupMode new_mode;
    std::chrono::system_clock::time_point request_time;
    std::string reason;

    PendingModeChange(DedupMode mode, const std::string &req_reason = "Configuration change")
        : new_mode(mode), request_time(std::chrono::system_clock::now()), reason(req_reason) {}
};

/**
 * @brief File processor that integrates file scanning, media processing, and database storage
 *
 * This class observes file names emitted by FileUtils::listFilesInternal,
 * processes them with MediaProcessor using the current quality setting,
 * and stores the results in a SQLite database.
 *
 * It also safely handles dedup mode changes by queuing them until current
 * processing completes and validating mode transitions.
 */
class FileProcessor : public ConfigObserver
{
public:
    /**
     * @brief Constructor
     * @param db_path Path to SQLite database file
     */
    explicit FileProcessor(const std::string &db_path);

    /**
     * @brief Destructor
     */
    ~FileProcessor();

    /**
     * @brief Process all files in a directory with current quality settings
     * @param dir_path Directory path to scan
     * @param recursive Whether to scan recursively
     * @return Number of files processed
     */
    size_t processDirectory(const std::string &dir_path, bool recursive = true);

    /**
     * @brief Process a single file with current quality settings
     * @param dir_path Path to file to process
     * @return FileProcessResult with success flag and error message
     */
    FileProcessResult processFile(const std::string &file_path);

    /**
     * @brief Get processing statistics
     * @return Pair of (total_files, successful_files)
     */
    std::pair<size_t, size_t> getProcessingStats() const;

    /**
     * @brief Clear processing statistics
     */
    void clearStats();

    /**
     * @brief Get the category of a file (Audio, Image, or Video)
     * @param file_path Path to the file
     * @return std::string category name
     */
    static std::string getFileCategory(const std::string &file_path);

    /**
     * @brief Wait for all pending database writes to complete
     */
    void waitForWrites();

    /**
     * @brief Handle configuration changes (ConfigObserver implementation)
     * @param event Configuration change event
     */
    void onConfigUpdate(const ConfigUpdateEvent &event) override;

    /**
     * @brief Check if file processing is currently active
     * @return true if processing is active, false otherwise
     */
    bool isProcessing() const;

    /**
     * @brief Get the current dedup mode being used for processing
     * @return Current dedup mode
     */
    DedupMode getCurrentDedupMode() const;

    /**
     * @brief Get pending mode change requests
     * @return Vector of pending mode changes
     */
    std::vector<PendingModeChange> getPendingModeChanges() const;

private:
    DatabaseManager *db_manager_;
    size_t total_files_processed_;
    size_t successful_files_processed_;

    // Processing state tracking
    mutable std::mutex processing_state_mutex_;
    std::atomic<bool> is_processing_active_;
    std::unordered_set<std::string> currently_processing_files_;

    // Mode change management
    mutable std::mutex mode_change_mutex_;
    std::queue<PendingModeChange> pending_mode_changes_;
    DedupMode current_dedup_mode_;

    // Cache management
    mutable std::mutex cache_mutex_;
    bool cache_clear_required_;

    /**
     * @brief Handle a single file from the file stream
     * @param file_path Path to the file
     */
    void handleFile(const std::string &file_path);

    /**
     * @brief Handle processing errors
     * @param error Exception that occurred
     */
    void handleError(const std::exception &error);

    /**
     * @brief Handle completion of file processing
     */
    void handleComplete();

    /**
     * @brief Check if dedup mode change requires cache clearing
     * @param old_mode Previous dedup mode
     * @param new_mode New dedup mode
     * @return true if cache clearing is required
     */
    bool requiresCacheClearing(DedupMode old_mode, DedupMode new_mode) const;

    /**
     * @brief Validate mode transition
     * @param old_mode Previous dedup mode
     * @param new_mode New dedup mode
     * @return true if transition is valid
     */
    bool isValidModeTransition(DedupMode old_mode, DedupMode new_mode) const;

    /**
     * @brief Queue mode change for later application
     * @param new_mode New dedup mode to apply
     * @param reason Reason for the mode change
     */
    void queueModeChange(DedupMode new_mode, const std::string &reason = "Configuration change");

    /**
     * @brief Apply pending mode changes when safe to do so
     */
    void applyPendingModeChanges();

    /**
     * @brief Clear decoder cache if required
     */
    void clearDecoderCacheIfRequired();

    /**
     * @brief Log mode change with audit trail
     * @param old_mode Previous dedup mode
     * @param new_mode New dedup mode
     * @param reason Reason for change
     */
    void logModeChange(DedupMode old_mode, DedupMode new_mode, const std::string &reason);

    /**
     * @brief Start processing a file
     * @param file_path Path to the file being processed
     */
    void startProcessingFile(const std::string &file_path);

    /**
     * @brief Finish processing a file
     * @param file_path Path to the file that finished processing
     */
    void finishProcessingFile(const std::string &file_path);
};