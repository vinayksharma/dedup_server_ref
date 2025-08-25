#include "core/transcoding_manager.hpp"
#include "core/media_processor.hpp"
#include "core/file_utils.hpp"
#include <filesystem>
#include <algorithm>
#include <cstdlib>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <iostream> // Required for std::cerr
#include "poco_config_adapter.hpp"
#include <libraw/libraw.h>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <algorithm> // Required for std::clamp and std::max
#include <cstring>   // Required for std::memcpy
#include <memory>    // For smart pointers
#include <unistd.h>

// FIXED: RAII wrapper for LibRaw to prevent memory leaks
class LibRawRAII
{
private:
    LibRaw *raw_;
    libraw_processed_image_t *img_;

public:
    LibRawRAII() : raw_(nullptr), img_(nullptr) {}

    ~LibRawRAII() { cleanup(); }

    // Disable copy constructor and assignment
    LibRawRAII(const LibRawRAII &) = delete;
    LibRawRAII &operator=(const LibRawRAII &) = delete;

    // Allow move constructor and assignment
    LibRawRAII(LibRawRAII &&other) noexcept : raw_(other.raw_), img_(other.img_)
    {
        other.raw_ = nullptr;
        other.img_ = nullptr;
    }

    LibRawRAII &operator=(LibRawRAII &&other) noexcept
    {
        if (this != &other)
        {
            cleanup();
            raw_ = other.raw_;
            img_ = other.img_;
            other.raw_ = nullptr;
            other.img_ = nullptr;
        }
        return *this;
    }

    void cleanup()
    {
        if (img_)
        {
            try
            {
                LibRaw::dcraw_clear_mem(img_);
            }
            catch (...)
            {
                // Ignore cleanup errors
            }
            img_ = nullptr;
        }

        if (raw_)
        {
            try
            {
                raw_->recycle();
                delete raw_;
            }
            catch (...)
            {
                // Ignore cleanup errors
            }
            raw_ = nullptr;
        }
    }

    LibRaw *getRaw() { return raw_; }
    libraw_processed_image_t *getImg() { return img_; }

    void setRaw(LibRaw *r) { raw_ = r; }
    void setImg(libraw_processed_image_t *i) { img_ = i; }

    // Check if resources are valid
    bool isValid() const { return raw_ != nullptr; }
};

// Raw file extensions that need transcoding - now configuration-driven
// These are no longer used as we use PocoConfigAdapter::needsTranscoding()

TranscodingManager &TranscodingManager::getInstance()
{
    static TranscodingManager instance;

    // Ensure the instance is properly initialized before returning
    if (!instance.isInitialized())
    {
        Logger::warn("TranscodingManager instance accessed before full initialization");
        // Return the instance anyway, but the methods will handle the uninitialized case
    }

    return instance;
}

void TranscodingManager::initialize(const std::string &cache_dir, int max_threads)
{
    Logger::info("Initializing TranscodingManager with cache dir: " + cache_dir +
                 ", max threads: " + std::to_string(max_threads));

    cache_dir_ = cache_dir;
    max_threads_ = max_threads;

    if (!db_manager_)
    {
        Logger::error("Database manager not available for transcoding initialization");
        return;
    }

    // Ensure cache directory exists
    if (!std::filesystem::exists(cache_dir_))
    {
        try
        {
            std::filesystem::create_directories(cache_dir_);
            Logger::info("Created cache directory: " + cache_dir_);
        }
        catch (const std::exception &e)
        {
            Logger::error("Failed to create cache directory: " + cache_dir_ + " - " + std::string(e.what()));
            return;
        }
    }

    // Upgrade database schema to add status and worker_id fields to cache_map table
    if (!upgradeCacheMapSchema())
    {
        Logger::error("Failed to upgrade cache_map table schema");
        return;
    }

    // Load configuration
    loadConfiguration();

    // Mark as fully initialized
    initialized_ = true;

    Logger::info("TranscodingManager initialized successfully");
}

void TranscodingManager::loadConfiguration()
{
    try
    {
        PocoConfigAdapter &config = PocoConfigAdapter::getInstance();

        // Load raw file extensions that need transcoding
        auto transcoding_types = config.getTranscodingFileTypes();
        for (const auto &[ext, enabled] : transcoding_types)
        {
            if (enabled)
            {
                raw_extensions_.push_back(ext);
            }
        }

        // Load cache size configuration
        size_t max_cache_mb = config.getDecoderCacheSizeMB();
        max_cache_size_mb_ = max_cache_mb;
        max_cache_size_.store(max_cache_mb * 1024 * 1024); // Convert MB to bytes

        // Set reasonable defaults for cleanup configuration
        cleanup_config_.fully_processed_age_days = 30;    // 30 days
        cleanup_config_.partially_processed_age_days = 7; // 7 days
        cleanup_config_.unprocessed_age_days = 3;         // 3 days
        cleanup_config_.require_all_modes = false;        // Don't require all modes
        cleanup_config_.cleanup_threshold_percent = 80;   // 80% threshold

        Logger::info("Loaded transcoding configuration: " +
                     std::string("Raw extensions: ") + std::to_string(raw_extensions_.size()) + ", " +
                     std::string("Max cache: ") + std::to_string(max_cache_mb) + "MB, " +
                     std::string("Fully processed: ") + std::to_string(cleanup_config_.fully_processed_age_days) + " days, " +
                     std::string("Partially processed: ") + std::to_string(cleanup_config_.partially_processed_age_days) + " days, " +
                     std::string("Unprocessed: ") + std::to_string(cleanup_config_.unprocessed_age_days) + " days, " +
                     std::string("Require all modes: ") + (cleanup_config_.require_all_modes ? "true" : "false") + ", " +
                     std::string("Cleanup threshold: ") + std::to_string(cleanup_config_.cleanup_threshold_percent) + "%");
    }
    catch (const std::exception &e)
    {
        Logger::error("Failed to load transcoding configuration: " + std::string(e.what()));
        // Use default values if configuration loading fails
        cleanup_config_.fully_processed_age_days = 30;
        cleanup_config_.partially_processed_age_days = 7;
        cleanup_config_.unprocessed_age_days = 3;
        cleanup_config_.require_all_modes = false;
        cleanup_config_.cleanup_threshold_percent = 80;
    }
}

void TranscodingManager::restoreQueueFromDatabase()
{
    Logger::info("Restoring transcoding queue from database...");

    if (!db_manager_)
    {
        Logger::error("Database manager not available for queue restoration");
        return;
    }

    try
    {
        // Query for files that need transcoding (status = 0, not yet processed)
        std::string sql = "SELECT source_file_path FROM cache_map WHERE status = 0";
        sqlite3_stmt *stmt;

        if (sqlite3_prepare_v2(db_manager_->db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
        {
            Logger::error("Failed to prepare queue restoration statement");
            return;
        }

        int restored_count = 0;
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            std::string source_file = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
            if (!source_file.empty())
            {
                // Check if the source file still exists
                if (std::filesystem::exists(source_file))
                {
                    // Check if it's not already transcoded
                    std::string transcoded_path = getTranscodedFilePath(source_file);
                    if (transcoded_path.empty())
                    {
                        // Queue for transcoding
                        queueForTranscoding(source_file);
                        restored_count++;
                    }
                }
            }
        }

        sqlite3_finalize(stmt);
        Logger::info("Restored " + std::to_string(restored_count) + " files to transcoding queue");
    }
    catch (const std::exception &e)
    {
        Logger::error("Exception during queue restoration: " + std::string(e.what()));
    }
}

void TranscodingManager::resetTranscodingJobStatusesOnStartup()
{
    Logger::info("Resetting all transcoding job statuses from 1 (in progress) to 0 (queued) on startup");

    if (!db_manager_)
    {
        Logger::error("Database manager not available for status reset");
        return;
    }

    try
    {
        // Reset all transcoding job statuses from 1 (in progress) to 0 (queued)
        std::string update_sql = "UPDATE cache_map SET status = 0, worker_id = NULL WHERE status = 1";

        if (sqlite3_exec(db_manager_->db_, update_sql.c_str(), nullptr, nullptr, nullptr) != SQLITE_OK)
        {
            Logger::error("Failed to reset transcoding job statuses: " + std::string(sqlite3_errmsg(db_manager_->db_)));
            return;
        }

        // Get count of affected rows
        std::string count_sql = "SELECT COUNT(*) FROM cache_map WHERE status = 1";
        sqlite3_stmt *count_stmt;

        if (sqlite3_prepare_v2(db_manager_->db_, count_sql.c_str(), -1, &count_stmt, nullptr) == SQLITE_OK)
        {
            if (sqlite3_step(count_stmt) == SQLITE_ROW)
            {
                int remaining_count = sqlite3_column_int(count_stmt, 0);
                if (remaining_count > 0)
                {
                    Logger::warn("Warning: " + std::to_string(remaining_count) + " transcoding jobs still have status 1 after reset");
                }
            }
            sqlite3_finalize(count_stmt);
        }

        Logger::info("Successfully reset all transcoding job statuses on startup");
    }
    catch (const std::exception &e)
    {
        Logger::error("Exception during transcoding job status reset: " + std::string(e.what()));
    }
}

void TranscodingManager::setDatabaseManager(DatabaseManager *db_manager)
{
    db_manager_ = db_manager;
    Logger::info("Database manager set for transcoding manager");
}

bool TranscodingManager::upgradeCacheMapSchema()
{
    Logger::info("Upgrading cache_map table schema...");

    try
    {
        // Check if status column exists
        std::string check_sql = "PRAGMA table_info(cache_map)";
        sqlite3_stmt *check_stmt;

        if (sqlite3_prepare_v2(db_manager_->db_, check_sql.c_str(), -1, &check_stmt, nullptr) != SQLITE_OK)
        {
            Logger::error("Failed to prepare schema check statement");
            return false;
        }

        bool status_exists = false;
        bool worker_id_exists = false;
        bool created_at_exists = false;
        bool updated_at_exists = false;

        while (sqlite3_step(check_stmt) == SQLITE_ROW)
        {
            std::string column_name = reinterpret_cast<const char *>(sqlite3_column_text(check_stmt, 1));
            if (column_name == "status")
                status_exists = true;
            if (column_name == "worker_id")
                worker_id_exists = true;
            if (column_name == "created_at")
                created_at_exists = true;
            if (column_name == "updated_at")
                updated_at_exists = true;
        }
        sqlite3_finalize(check_stmt);

        // Add missing columns
        if (!status_exists)
        {
            std::string alter_sql = "ALTER TABLE cache_map ADD COLUMN status INTEGER DEFAULT 0";
            if (sqlite3_exec(db_manager_->db_, alter_sql.c_str(), nullptr, nullptr, nullptr) != SQLITE_OK)
            {
                Logger::error("Failed to add status column to cache_map table");
                return false;
            }
            Logger::info("Added status column to cache_map table");
        }

        if (!worker_id_exists)
        {
            std::string alter_sql = "ALTER TABLE cache_map ADD COLUMN worker_id TEXT";
            if (sqlite3_exec(db_manager_->db_, alter_sql.c_str(), nullptr, nullptr, nullptr) != SQLITE_OK)
            {
                Logger::error("Failed to add worker_id column to cache_map table");
                return false;
            }
            Logger::info("Added worker_id column to cache_map table");
        }

        if (!created_at_exists)
        {
            std::string alter_sql = "ALTER TABLE cache_map ADD COLUMN created_at DATETIME DEFAULT CURRENT_TIMESTAMP";
            if (sqlite3_exec(db_manager_->db_, alter_sql.c_str(), nullptr, nullptr, nullptr) != SQLITE_OK)
            {
                Logger::error("Failed to add created_at column to cache_map table");
                return false;
            }
            Logger::info("Added created_at column to cache_map table");
        }

        if (!updated_at_exists)
        {
            std::string alter_sql = "ALTER TABLE cache_map ADD COLUMN updated_at DATETIME DEFAULT CURRENT_TIMESTAMP";
            if (sqlite3_exec(db_manager_->db_, alter_sql.c_str(), nullptr, nullptr, nullptr) != SQLITE_OK)
            {
                Logger::error("Failed to add updated_at column to cache_map table");
                return false;
            }
            Logger::info("Added updated_at column to cache_map table");
        }

        // Create index on status for faster job selection
        std::string index_sql = "CREATE INDEX IF NOT EXISTS idx_cache_map_status ON cache_map(status, created_at)";
        if (sqlite3_exec(db_manager_->db_, index_sql.c_str(), nullptr, nullptr, nullptr) != SQLITE_OK)
        {
            Logger::warn("Failed to create index on cache_map status (may already exist)");
        }

        Logger::info("cache_map table schema upgrade completed successfully");
        return true;
    }
    catch (const std::exception &e)
    {
        Logger::error("Exception during schema upgrade: " + std::string(e.what()));
        return false;
    }
}

std::string TranscodingManager::getNextTranscodingJob()
{
    Logger::debug("getNextTranscodingJob called");

    if (!db_manager_)
    {
        Logger::error("Database manager not available for job selection");
        return "";
    }

    // Route through DatabaseManager access queue to serialize with other DB operations
    return db_manager_->claimNextTranscodingJob();
}

bool TranscodingManager::markJobInProgress(const std::string &file_path)
{
    if (!db_manager_ || file_path.empty())
    {
        return false;
    }
    return db_manager_->markTranscodingJobInProgress(file_path);
}

bool TranscodingManager::markJobCompleted(const std::string &file_path, const std::string &output_path)
{
    if (!db_manager_ || file_path.empty() || output_path.empty())
    {
        return false;
    }
    return db_manager_->markTranscodingJobCompleted(file_path, output_path);
}

bool TranscodingManager::markJobFailed(const std::string &file_path)
{
    if (!db_manager_ || file_path.empty())
    {
        return false;
    }
    return db_manager_->markTranscodingJobFailed(file_path);
}

void TranscodingManager::startTranscoding()
{
    if (running_.load())
    {
        Logger::warn("Transcoding is already running");
        return;
    }

    running_.store(true);
    cancelled_.store(false);

    // Start only ONE transcoding thread since we process files sequentially
    transcoding_threads_.emplace_back(&TranscodingManager::transcodingThread, this);

    Logger::info("Started 1 transcoding thread (database-only approach)");
}

void TranscodingManager::stopTranscoding()
{
    if (!running_.load())
    {
        return;
    }

    Logger::info("Stopping transcoding threads");

    cancelled_.store(true);
    queue_cv_.notify_all();

    // Wait for all threads to complete
    for (auto &thread : transcoding_threads_)
    {
        if (thread.joinable())
        {
            thread.join();
        }
    }

    transcoding_threads_.clear();
    running_.store(false);

    Logger::info("Transcoding threads stopped");
}

bool TranscodingManager::isRawFile(const std::string &file_path)
{
    std::string extension = MediaProcessor::getFileExtension(file_path);
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

    // Use PocoConfigAdapter to check if this extension needs transcoding
    bool needs_transcoding = PocoConfigAdapter::getInstance().needsTranscoding(extension);

    return needs_transcoding;
}

void TranscodingManager::queueForTranscoding(const std::string &file_path)
{
    Logger::debug("queueForTranscoding called for: " + file_path);

    if (!isInitialized())
    {
        Logger::warn("TranscodingManager not fully initialized for queueForTranscoding");
        return;
    }

    if (!running_.load())
    {
        Logger::warn("Transcoding not running, cannot queue file: " + file_path);
        return;
    }

    if (!isDatabaseAvailable())
    {
        Logger::error("Database manager not available for transcoding queue");
        return;
    }

    try
    {
        // Check if already transcoded (database check)
        std::string existing_transcoded = getTranscodedFilePath(file_path);
        if (!existing_transcoded.empty())
        {
            Logger::debug("File already transcoded: " + file_path + " -> " + existing_transcoded);
            return;
        }

        // Queue via DatabaseManager to ensure serialized DB access
        DBOpResult insert_result = db_manager_->insertTranscodingFile(file_path);
        if (!insert_result.success)
        {
            Logger::error("Failed to queue file for transcoding: " + file_path + " - " + insert_result.error_message);
            return;
        }

        Logger::info("Queued file for transcoding: " + file_path);
    }
    catch (const std::exception &e)
    {
        Logger::error("Exception queuing file for transcoding: " + std::string(e.what()));
    }
}

std::string TranscodingManager::getTranscodedFilePath(const std::string &source_file_path)
{
    if (!isInitialized())
    {
        Logger::warn("TranscodingManager not fully initialized for getTranscodedFilePath");
        return "";
    }
    if (!db_manager_)
    {
        Logger::warn("Database manager not available for getTranscodedFilePath");
        return "";
    }
    return db_manager_->getTranscodedFilePath(source_file_path);
}

// Helper method to check if database manager is available
bool TranscodingManager::isDatabaseAvailable() const
{
    return db_manager_ != nullptr;
}

// Helper method to safely access database manager
DatabaseManager *TranscodingManager::getDatabaseManager() const
{
    if (!db_manager_)
    {
        Logger::warn("Database manager not available");
        return nullptr;
    }
    return db_manager_;
}

bool TranscodingManager::isTranscodingRunning() const
{
    return running_.load();
}

std::pair<size_t, size_t> TranscodingManager::getTranscodingStats() const
{
    return {queued_count_.load(), completed_count_.load()};
}

void TranscodingManager::shutdown()
{
    stopTranscoding();
    Logger::info("TranscodingManager shutdown complete");
}

void TranscodingManager::transcodingThread()
{
    Logger::info("Transcoding thread started (database-only mode)");

    while (running_.load() && !cancelled_.load())
    {
        try
        {
            Logger::debug("Transcoding thread checking for jobs...");

            // Get next job from database
            std::string file_path = getNextTranscodingJob();

            if (file_path.empty())
            {
                // No jobs available, wait a bit before checking again
                Logger::debug("No jobs available, waiting...");
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                continue;
            }

            Logger::debug("Got job: " + file_path);

            // Mark job as in progress
            if (!markJobInProgress(file_path))
            {
                Logger::warn("Failed to mark job as in progress, skipping: " + file_path);
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }

            Logger::info("Processing transcoding job: " + file_path);

            // Attempt to transcode the file
            std::string output_path = transcodeFile(file_path);

            if (!output_path.empty())
            {
                // Transcoding succeeded
                if (markJobCompleted(file_path, output_path))
                {
                    processed_count_.fetch_add(1);
                    Logger::info("Transcoding completed successfully: " + file_path + " -> " + output_path);
                }
                else
                {
                    Logger::warn("Failed to mark job as completed: " + file_path);
                }
            }
            else
            {
                // Transcoding failed
                if (markJobFailed(file_path))
                {
                    failed_count_.fetch_add(1);
                    Logger::warn("Transcoding failed: " + file_path);
                }
                else
                {
                    Logger::warn("Failed to mark job as failed: " + file_path);
                }
            }

            // Small delay between jobs to prevent overwhelming the system
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        catch (const std::exception &e)
        {
            Logger::error("Exception in transcoding thread: " + std::string(e.what()));
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }

    Logger::info("Transcoding thread stopped");
}

std::string TranscodingManager::transcodeFile(const std::string &source_file_path)
{
    std::string cache_filename = generateCacheFilename(source_file_path);
    std::string output_path = std::filesystem::path(cache_dir_) / cache_filename;

    // Check if already exists and is valid
    if (std::filesystem::exists(output_path))
    {
        // Use existing file change detection to check if cache is still valid
        auto current_metadata = FileUtils::getFileMetadata(source_file_path);
        if (current_metadata)
        {
            // Check if source file has changed using existing metadata system
            // This would integrate with the existing DatabaseManager metadata comparison
            Logger::debug("Transcoded file already exists: " + output_path);
            return output_path;
        }
        else
        {
            // Source file doesn't exist, remove invalid cache entry
            Logger::debug("Source file no longer exists, removing invalid cache: " + output_path);
            std::filesystem::remove(output_path);
        }
    }

    // Check cache size and cleanup if needed
    if (isCacheOverLimit())
    {
        Logger::info("Cache size limit exceeded, performing smart cleanup before transcoding");
        cleanupCacheSmart(true);
    }

    // Use LibRaw directly for transcoding (no external executables)
    if (transcodeRawFileDirectly(source_file_path, output_path))
    {
        Logger::info("LibRaw transcoding succeeded: " + source_file_path + " -> " + output_path);
        return output_path;
    }
    Logger::error("LibRaw transcoding failed for: " + source_file_path);
    return "";
}

std::string TranscodingManager::generateCacheFilename(const std::string &source_file_path)
{
    // Generate a unique filename based on source file path hash
    std::string hash = MediaProcessor::generateHash(std::vector<uint8_t>(source_file_path.begin(), source_file_path.end()));

    // Get original extension and convert to jpg
    std::string original_ext = MediaProcessor::getFileExtension(source_file_path);
    std::transform(original_ext.begin(), original_ext.end(), original_ext.begin(), ::tolower);

    // Use first 16 characters of hash + original extension + .jpg
    return hash.substr(0, 16) + "_" + original_ext + ".jpg";
}

size_t TranscodingManager::getCacheSize() const
{
    std::lock_guard<std::mutex> lock(cache_size_mutex_);

    size_t total_size = 0;
    try
    {
        for (const auto &entry : std::filesystem::recursive_directory_iterator(cache_dir_))
        {
            if (entry.is_regular_file())
            {
                total_size += entry.file_size();
            }
        }
    }
    catch (const std::exception &e)
    {
        Logger::error("Error calculating cache size: " + std::string(e.what()));
    }

    return total_size;
}

std::string TranscodingManager::getCacheSizeString() const
{
    size_t size_bytes = getCacheSize();

    const char *units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit_index = 0;
    double size_double = static_cast<double>(size_bytes);

    while (size_double >= 1024.0 && unit_index < 4)
    {
        size_double /= 1024.0;
        unit_index++;
    }

    std::stringstream ss;
    ss << std::fixed << std::setprecision(1) << size_double << " " << units[unit_index];
    return ss.str();
}

void TranscodingManager::setMaxCacheSize(size_t max_size_bytes)
{
    max_cache_size_.store(max_size_bytes);
    Logger::info("Cache size limit set to: " + std::to_string(max_size_bytes) + " bytes");
}

size_t TranscodingManager::getMaxCacheSize() const
{
    return max_cache_size_.load();
}

void TranscodingManager::setCleanupConfig(int fully_processed_days, int partially_processed_days,
                                          int unprocessed_days, bool require_all_modes,
                                          int cleanup_threshold_percent)
{
    std::lock_guard<std::mutex> lock(cache_size_mutex_);

    cleanup_config_.fully_processed_age_days = std::max(1, fully_processed_days);
    cleanup_config_.partially_processed_age_days = std::max(1, partially_processed_days);
    cleanup_config_.unprocessed_age_days = std::max(1, unprocessed_days);
    cleanup_config_.require_all_modes = require_all_modes;
    cleanup_config_.cleanup_threshold_percent = std::clamp(cleanup_threshold_percent, 50, 95);

    Logger::info("Cache cleanup configuration updated: " +
                 std::string("Fully processed: ") + std::to_string(cleanup_config_.fully_processed_age_days) + " days, " +
                 std::string("Partially processed: ") + std::to_string(cleanup_config_.partially_processed_age_days) + " days, " +
                 std::string("Unprocessed: ") + std::to_string(cleanup_config_.unprocessed_age_days) + " days, " +
                 std::string("Require all modes: ") + (cleanup_config_.require_all_modes ? "true" : "false") + ", " +
                 std::string("Cleanup threshold: ") + std::to_string(cleanup_config_.cleanup_threshold_percent) + "%");
}

const TranscodingManager::CleanupConfig &TranscodingManager::getCleanupConfig() const
{
    return cleanup_config_;
}

bool TranscodingManager::isCacheOverLimit() const
{
    return getCacheSize() > getMaxCacheSize();
}

size_t TranscodingManager::cleanupCache(bool force_cleanup)
{
    std::lock_guard<std::mutex> lock(cache_size_mutex_);

    size_t current_size = getCacheSize();
    size_t max_size = getMaxCacheSize();

    if (!force_cleanup && current_size <= max_size)
    {
        Logger::debug("Cache size (" + getCacheSizeString() + ") is under limit, no cleanup needed");
        return 0;
    }

    Logger::info("Starting cache cleanup. Current size: " + getCacheSizeString() +
                 ", Max size: " + std::to_string(max_size / (1024 * 1024)) + " MB");

    // Get all cache files and sort by modification time (oldest first)
    std::vector<std::filesystem::path> cache_files;
    try
    {
        for (const auto &entry : std::filesystem::recursive_directory_iterator(cache_dir_))
        {
            if (entry.is_regular_file())
            {
                cache_files.push_back(entry.path());
            }
        }
    }
    catch (const std::exception &e)
    {
        Logger::error("Error scanning cache directory: " + std::string(e.what()));
        return 0;
    }

    // Sort by modification time (oldest first)
    std::sort(cache_files.begin(), cache_files.end(),
              [](const auto &a, const auto &b)
              {
                  return std::filesystem::last_write_time(a) < std::filesystem::last_write_time(b);
              });

    // Remove oldest files until under limit
    size_t files_removed = 0;
    size_t bytes_freed = 0;

    for (const auto &cache_file : cache_files)
    {
        if (current_size <= max_size)
        {
            break;
        }

        try
        {
            size_t file_size = std::filesystem::file_size(cache_file);
            std::filesystem::remove(cache_file);

            current_size -= file_size;
            bytes_freed += file_size;
            files_removed++;

            Logger::debug("Removed cache file: " + cache_file.string() +
                          " (size: " + std::to_string(file_size) + " bytes)");
        }
        catch (const std::exception &e)
        {
            Logger::error("Error removing cache file " + cache_file.string() + ": " + std::string(e.what()));
        }
    }

    Logger::info("Cache cleanup completed. Removed " + std::to_string(files_removed) +
                 " files, freed " + std::to_string(bytes_freed / (1024 * 1024)) + " MB");

    return files_removed;
}

size_t TranscodingManager::cleanupCacheEnhanced(bool force_cleanup)
{
    std::lock_guard<std::mutex> lock(cache_size_mutex_);

    size_t current_size = getCacheSize();
    size_t max_size = getMaxCacheSize();

    if (!force_cleanup && current_size <= max_size)
    {
        Logger::debug("Cache size (" + getCacheSizeString() + ") is under limit, no cleanup needed");
        return 0;
    }

    Logger::info("Starting enhanced cache cleanup. Current size: " + getCacheSizeString() +
                 ", Max size: " + std::to_string(max_size / (1024 * 1024)) + " MB");

    // Get all cache entries from database
    std::vector<std::pair<std::string, std::string>> cache_entries;
    try
    {
        // This would come from DatabaseManager::getAllCacheEntries()
        // For now, we'll scan the cache directory
        for (const auto &entry : std::filesystem::recursive_directory_iterator(cache_dir_))
        {
            if (entry.is_regular_file())
            {
                // Extract source file path from cache filename (reverse the hash)
                // This is simplified - in practice, we'd store the mapping in database
                std::string cache_file = entry.path().string();
                std::string source_file = "unknown"; // Would be retrieved from database

                cache_entries.emplace_back(source_file, cache_file);
            }
        }
    }
    catch (const std::exception &e)
    {
        Logger::error("Error scanning cache directory: " + std::string(e.what()));
        return 0;
    }

    // Use existing file change detection system
    std::vector<std::pair<std::string, std::string>> invalid_files; // Source file changed
    std::vector<std::pair<std::string, std::string>> valid_files;   // Source file unchanged

    for (const auto &[source_file, cache_file] : cache_entries)
    {
        // Use existing FileUtils::getFileMetadata and database comparison
        auto current_metadata = FileUtils::getFileMetadata(source_file);
        if (!current_metadata)
        {
            // Source file doesn't exist, mark as invalid
            invalid_files.emplace_back(source_file, cache_file);
            continue;
        }

        // Get stored metadata from database (this would be implemented in DatabaseManager)
        // For now, we'll assume files are valid if they exist
        if (std::filesystem::exists(cache_file))
        {
            valid_files.emplace_back(source_file, cache_file);
        }
        else
        {
            invalid_files.emplace_back(source_file, cache_file);
        }
    }

    Logger::info("Cache analysis: " + std::to_string(valid_files.size()) + " valid files, " +
                 std::to_string(invalid_files.size()) + " invalid files");

    // Remove invalid files first (source file changed or doesn't exist)
    size_t files_removed = 0;
    size_t bytes_freed = 0;

    for (const auto &[source_file, cache_file] : invalid_files)
    {
        try
        {
            size_t file_size = std::filesystem::file_size(cache_file);
            std::filesystem::remove(cache_file);

            current_size -= file_size;
            bytes_freed += file_size;
            files_removed++;

            Logger::debug("Removed invalid cache file: " + cache_file +
                          " (source changed or missing, size: " + std::to_string(file_size) + " bytes)");
        }
        catch (const std::exception &e)
        {
            Logger::error("Error removing invalid cache file " + cache_file + ": " + std::string(e.what()));
        }
    }

    // If still over limit, remove oldest valid files
    if (current_size > max_size)
    {
        Logger::info("Still over limit after removing invalid files, removing oldest valid files");

        // Sort valid files by modification time (oldest first)
        std::sort(valid_files.begin(), valid_files.end(),
                  [](const auto &a, const auto &b)
                  {
                      return std::filesystem::last_write_time(a.second) < std::filesystem::last_write_time(b.second);
                  });

        // Remove oldest files until under limit
        for (const auto &[source_file, cache_file] : valid_files)
        {
            if (current_size <= max_size)
            {
                break;
            }

            try
            {
                size_t file_size = std::filesystem::file_size(cache_file);
                std::filesystem::remove(cache_file);

                current_size -= file_size;
                bytes_freed += file_size;
                files_removed++;

                Logger::debug("Removed oldest valid cache file: " + cache_file +
                              " (size: " + std::to_string(file_size) + " bytes)");
            }
            catch (const std::exception &e)
            {
                Logger::error("Error removing oldest cache file " + cache_file + ": " + std::string(e.what()));
            }
        }
    }

    Logger::info("Enhanced cache cleanup completed. Removed " + std::to_string(files_removed) +
                 " files, freed " + std::to_string(bytes_freed / (1024 * 1024)) + " MB");

    return files_removed;
}

size_t TranscodingManager::cleanupCacheSmart(bool force_cleanup)
{
    std::lock_guard<std::mutex> lock(cache_size_mutex_);

    size_t current_size = getCacheSize();
    size_t max_size = getMaxCacheSize();

    if (!force_cleanup && current_size <= max_size)
    {
        Logger::debug("Cache size (" + getCacheSizeString() + ") is under limit, no cleanup needed");
        return 0;
    }

    Logger::info("Starting smart cache cleanup. Current size: " + getCacheSizeString() +
                 ", Max size: " + std::to_string(max_size / (1024 * 1024)) + " MB");

    // Get cache entries with processing status from database
    std::vector<CacheEntry> cache_entries = getCacheEntriesWithStatus();

    if (cache_entries.empty())
    {
        Logger::info("No cache entries found for smart cleanup");
        return 0;
    }

    Logger::info("Analyzing " + std::to_string(cache_entries.size()) + " cache entries for smart cleanup");

    // Phase 1: Remove invalid files (source changed/missing)
    size_t invalid_removed = removeInvalidFiles(cache_entries);
    Logger::info("Phase 1 completed: Removed " + std::to_string(invalid_removed) + " invalid files");

    // Phase 2: Remove processed old files
    size_t processed_removed = removeProcessedOldFiles(cache_entries);
    Logger::info("Phase 2 completed: Removed " + std::to_string(processed_removed) + " processed old files");

    // Phase 3: Remove unprocessed old files
    size_t unprocessed_removed = removeUnprocessedOldFiles(cache_entries);
    Logger::info("Phase 3 completed: Removed " + std::to_string(unprocessed_removed) + " unprocessed old files");

    // Phase 4: If still over limit, remove oldest valid files
    size_t oldest_removed = 0;
    if (getCacheSize() > max_size)
    {
        Logger::info("Still over limit after smart cleanup, removing oldest valid files");
        oldest_removed = removeOldestValidFiles(cache_entries);
        Logger::info("Phase 4 completed: Removed " + std::to_string(oldest_removed) + " oldest valid files");
    }

    size_t total_removed = invalid_removed + processed_removed + unprocessed_removed + oldest_removed;
    size_t final_size = getCacheSize();

    Logger::info("Smart cache cleanup completed. Total files removed: " + std::to_string(total_removed) +
                 ", Final cache size: " + getCacheSizeString() +
                 ", Space freed: " + std::to_string((current_size - final_size) / (1024 * 1024)) + " MB");

    return total_removed;
}

std::vector<TranscodingManager::CacheEntry> TranscodingManager::getCacheEntriesWithStatus()
{
    std::vector<CacheEntry> entries;

    try
    {
        if (!db_manager_)
        {
            Logger::error("Database manager not available for cache status lookup");
            return entries;
        }

        // This would be implemented in DatabaseManager to get cache entries with processing status
        // For now, we'll scan the cache directory and build entries manually
        for (const auto &entry : std::filesystem::recursive_directory_iterator(cache_dir_))
        {
            if (entry.is_regular_file())
            {
                std::string cache_file = entry.path().string();

                // Extract source file path from cache filename (reverse the hash)
                // This is simplified - in practice, we'd get this from database
                std::string source_file = "unknown"; // Would be retrieved from database

                // Get file metadata
                auto current_metadata = FileUtils::getFileMetadata(source_file);
                bool source_exists = current_metadata.has_value();

                // Get processing status from database (would be implemented)
                bool processed_fast = false;
                bool processed_balanced = false;
                bool processed_quality = false;

                // Determine processing status
                bool is_processed = processed_fast || processed_balanced || processed_quality;
                bool is_fully_processed = processed_fast && processed_balanced && processed_quality;

                // Calculate cache age
                std::time_t cache_age = 0;
                try
                {
                    auto cache_time = std::filesystem::last_write_time(entry.path());
                    auto now = std::chrono::system_clock::now();
                    auto cache_duration = cache_time.time_since_epoch();
                    auto now_duration = now.time_since_epoch();
                    auto age_duration = now_duration - cache_duration;
                    cache_age = std::chrono::duration_cast<std::chrono::seconds>(age_duration).count();
                }
                catch (...)
                {
                    cache_age = std::time(nullptr);
                }

                // Get file size
                size_t file_size = std::filesystem::file_size(entry.path());

                // Create processing status string
                std::string processing_status;
                if (!source_exists)
                {
                    processing_status = "SOURCE_MISSING";
                }
                else if (is_fully_processed)
                {
                    processing_status = "FULLY_PROCESSED";
                }
                else if (is_processed)
                {
                    processing_status = "PARTIALLY_PROCESSED";
                }
                else
                {
                    processing_status = "UNPROCESSED";
                }

                CacheEntry cache_entry{
                    source_file,
                    cache_file,
                    is_processed,
                    is_fully_processed,
                    cache_age,
                    file_size,
                    processing_status};

                entries.push_back(cache_entry);
            }
        }
    }
    catch (const std::exception &e)
    {
        Logger::error("Error getting cache entries with status: " + std::string(e.what()));
    }

    return entries;
}

size_t TranscodingManager::removeInvalidFiles(const std::vector<CacheEntry> &entries)
{
    size_t files_removed = 0;

    for (const auto &entry : entries)
    {
        if (entry.processing_status == "SOURCE_MISSING")
        {
            if (removeCacheEntry(entry))
            {
                files_removed++;
                Logger::debug("Removed invalid cache file (source missing): " + entry.cache_file);
            }
        }
    }

    return files_removed;
}

size_t TranscodingManager::removeProcessedOldFiles(const std::vector<CacheEntry> &entries)
{
    size_t files_removed = 0;

    for (const auto &entry : entries)
    {
        if (entry.is_fully_processed && isOldEnoughForCleanup(entry))
        {
            if (removeCacheEntry(entry))
            {
                files_removed++;
                Logger::debug("Removed processed old cache file: " + entry.cache_file +
                              " (age: " + std::to_string(entry.cache_age) + "s, status: " + entry.processing_status + ")");
            }
        }
    }

    return files_removed;
}

size_t TranscodingManager::removeUnprocessedOldFiles(const std::vector<CacheEntry> &entries)
{
    size_t files_removed = 0;

    for (const auto &entry : entries)
    {
        if (!entry.is_processed && isOldEnoughForCleanup(entry))
        {
            if (removeCacheEntry(entry))
            {
                files_removed++;
                Logger::debug("Removed unprocessed old cache file: " + entry.cache_file +
                              " (age: " + std::to_string(entry.cache_age) + "s, status: " + entry.processing_status + ")");
            }
        }
    }

    return files_removed;
}

size_t TranscodingManager::removeOldestValidFiles(const std::vector<CacheEntry> &entries)
{
    size_t files_removed = 0;
    size_t current_size = getCacheSize();
    size_t max_size = getMaxCacheSize();

    // Filter valid entries (source exists, not already removed)
    std::vector<CacheEntry> valid_entries;
    for (const auto &entry : entries)
    {
        if (entry.processing_status != "SOURCE_MISSING" &&
            std::filesystem::exists(entry.cache_file))
        {
            valid_entries.push_back(entry);
        }
    }

    // Sort by cache age (oldest first)
    std::sort(valid_entries.begin(), valid_entries.end(),
              [](const auto &a, const auto &b)
              {
                  return a.cache_age < b.cache_age;
              });

    // Remove oldest files until under limit
    for (const auto &entry : valid_entries)
    {
        if (current_size <= max_size)
        {
            break;
        }

        if (removeCacheEntry(entry))
        {
            current_size -= entry.file_size;
            files_removed++;
            Logger::debug("Removed oldest valid cache file: " + entry.cache_file +
                          " (age: " + std::to_string(entry.cache_age) + "s, size: " + std::to_string(entry.file_size) + " bytes)");
        }
    }

    return files_removed;
}

bool TranscodingManager::isOldEnoughForCleanup(const CacheEntry &entry) const
{
    auto now = std::time(nullptr);
    auto age_seconds = now - entry.cache_age;

    if (entry.is_fully_processed)
    {
        // Fully processed files: remove after configured age
        return age_seconds > (cleanup_config_.fully_processed_age_days * 24 * 3600);
    }
    else if (entry.is_processed)
    {
        // Partially processed files: remove after configured age
        return age_seconds > (cleanup_config_.partially_processed_age_days * 24 * 3600);
    }
    else
    {
        // Unprocessed files: remove after configured age
        return age_seconds > (cleanup_config_.unprocessed_age_days * 24 * 3600);
    }
}

bool TranscodingManager::removeCacheEntry(const CacheEntry &entry)
{
    try
    {
        // Remove cache file
        if (std::filesystem::exists(entry.cache_file))
        {
            std::filesystem::remove(entry.cache_file);
        }

        // Remove database record
        if (db_manager_)
        {
            DBOpResult result = db_manager_->removeTranscodingRecord(entry.source_file);
            if (!result.success)
            {
                Logger::warn("Failed to remove transcoding record from database: " + entry.source_file +
                             " - " + result.error_message);
            }
        }

        return true;
    }
    catch (const std::exception &e)
    {
        Logger::error("Error removing cache entry: " + entry.cache_file + " - " + std::string(e.what()));
        return false;
    }
}

bool TranscodingManager::transcodeRawFileDirectly(const std::string &source_file_path, const std::string &output_path)
{
    std::lock_guard<std::mutex> lock(libraw_mutex_);

    LibRawRAII libraw_raii;

    try
    {
        // Validate input file exists and is readable
        if (!std::filesystem::exists(source_file_path))
        {
            Logger::error("Source file does not exist: " + source_file_path);
            return false;
        }

        // Ensure parent directory exists
        std::filesystem::create_directories(std::filesystem::path(output_path).parent_path());

        // Create LibRaw instance
        libraw_raii.setRaw(new LibRaw());
        if (!libraw_raii.getRaw())
        {
            Logger::error("Failed to create LibRaw instance for: " + source_file_path);
            return false;
        }

        // Configure LibRaw parameters
        libraw_raii.getRaw()->imgdata.params.use_camera_wb = 1;
        libraw_raii.getRaw()->imgdata.params.use_auto_wb = 0;
        libraw_raii.getRaw()->imgdata.params.no_auto_bright = 1;
        libraw_raii.getRaw()->imgdata.params.output_bps = 8;
        libraw_raii.getRaw()->imgdata.params.output_color = 1; // sRGB
        libraw_raii.getRaw()->imgdata.params.half_size = 0;
        libraw_raii.getRaw()->imgdata.params.output_tiff = 0; // JPEG output

        Logger::debug("Opening RAW file: " + source_file_path);
        int rc = libraw_raii.getRaw()->open_file(source_file_path.c_str());
        if (rc != LIBRAW_SUCCESS)
        {
            Logger::error("LibRaw open_file failed: " + std::string(libraw_strerror(rc)) + " (" + std::to_string(rc) + ") for: " + source_file_path);
            return false;
        }

        Logger::debug("Unpacking RAW data for: " + source_file_path);
        rc = libraw_raii.getRaw()->unpack();
        if (rc != LIBRAW_SUCCESS)
        {
            Logger::error("LibRaw unpack failed: " + std::string(libraw_strerror(rc)) + " (" + std::to_string(rc) + ") for: " + source_file_path);
            return false;
        }

        Logger::debug("Processing RAW data for: " + source_file_path);
        rc = libraw_raii.getRaw()->dcraw_process();
        if (rc != LIBRAW_SUCCESS)
        {
            Logger::error("LibRaw dcraw_process failed: " + std::string(libraw_strerror(rc)) + " (" + std::to_string(rc) + ") for: " + source_file_path);
            return false;
        }

        Logger::debug("Creating memory image for: " + source_file_path);
        libraw_processed_image_t *temp_img = libraw_raii.getRaw()->dcraw_make_mem_image(&rc);
        if (!temp_img || rc != LIBRAW_SUCCESS)
        {
            Logger::error("LibRaw dcraw_make_mem_image failed: " + std::string(libraw_strerror(rc)) + " (" + std::to_string(rc) + ") for: " + source_file_path);
            return false;
        }
        libraw_raii.setImg(temp_img);

        // Validate image buffer
        if (libraw_raii.getImg()->type != LIBRAW_IMAGE_BITMAP)
        {
            Logger::error("Unsupported image type: " + std::to_string(libraw_raii.getImg()->type) + " for: " + source_file_path);
            return false;
        }

        if (libraw_raii.getImg()->colors != 3)
        {
            Logger::error("Unsupported color channels: " + std::to_string(libraw_raii.getImg()->colors) + " for: " + source_file_path);
            return false;
        }

        if (libraw_raii.getImg()->bits != 8)
        {
            Logger::error("Unsupported bit depth: " + std::to_string(libraw_raii.getImg()->bits) + " for: " + source_file_path);
            return false;
        }

        if (libraw_raii.getImg()->width <= 0 || libraw_raii.getImg()->height <= 0)
        {
            Logger::error("Invalid image dimensions: " + std::to_string(libraw_raii.getImg()->width) + "x" + std::to_string(libraw_raii.getImg()->height) + " for: " + source_file_path);
            return false;
        }

        // Note: img->data is an array, so it's always valid if img exists

        Logger::debug("Creating OpenCV Mat for: " + source_file_path + " (" + std::to_string(libraw_raii.getImg()->width) + "x" + std::to_string(libraw_raii.getImg()->height) + ")");

        // Create a copy of the data to avoid memory issues
        size_t data_size = libraw_raii.getImg()->width * libraw_raii.getImg()->height * 3;
        std::vector<unsigned char> rgb_data(data_size);
        std::memcpy(rgb_data.data(), libraw_raii.getImg()->data, data_size);

        // Construct cv::Mat with copied data (RGB to BGR for OpenCV)
        cv::Mat rgb(libraw_raii.getImg()->height, libraw_raii.getImg()->width, CV_8UC3, rgb_data.data());
        cv::Mat bgr;
        cv::cvtColor(rgb, bgr, cv::COLOR_RGB2BGR);

        Logger::debug("Writing JPEG output: " + output_path);
        std::vector<int> params = {cv::IMWRITE_JPEG_QUALITY, 92};
        if (!cv::imwrite(output_path, bgr, params))
        {
            Logger::error("OpenCV imwrite failed for: " + output_path);
            return false;
        }

        Logger::info("Successfully transcoded: " + source_file_path + " -> " + output_path);
        return true;
    }
    catch (const std::exception &e)
    {
        Logger::error("Exception during LibRaw transcoding: " + std::string(e.what()) + " for: " + source_file_path);
        return false;
    }
    catch (...)
    {
        Logger::error("Unknown exception during LibRaw transcoding for: " + source_file_path);
        return false;
    }

    // Cleanup section - always executed
    libraw_raii.cleanup();

    return false;
}

size_t TranscodingManager::retryTranscodingErrorFiles()
{
    Logger::info("Checking for files in transcoding error state (3) to retry...");

    if (!db_manager_)
    {
        Logger::error("Database manager not available for retry operation");
        return 0;
    }

    try
    {
        // Get files that are in transcoding error state (3) for any mode
        auto &config_manager = PocoConfigAdapter::getInstance();
        bool pre_process_quality_stack = config_manager.getPreProcessQualityStack();

        std::vector<std::string> files_to_retry;

        if (pre_process_quality_stack)
        {
            // Check all three modes for transcoding error state (3)
            auto fast_error_files = db_manager_->getFilesWithProcessingFlag(3, DedupMode::FAST);
            auto balanced_error_files = db_manager_->getFilesWithProcessingFlag(3, DedupMode::BALANCED);
            auto quality_error_files = db_manager_->getFilesWithProcessingFlag(3, DedupMode::QUALITY);

            // Combine all files that need retry
            files_to_retry.insert(files_to_retry.end(), fast_error_files.begin(), fast_error_files.end());
            files_to_retry.insert(files_to_retry.end(), balanced_error_files.begin(), balanced_error_files.end());
            files_to_retry.insert(files_to_retry.end(), quality_error_files.begin(), quality_error_files.end());
        }
        else
        {
            // Check only current mode
            DedupMode current_mode = config_manager.getDedupMode();
            files_to_retry = db_manager_->getFilesWithProcessingFlag(3, current_mode);
        }

        if (files_to_retry.empty())
        {
            Logger::debug("No files in transcoding error state (3) found for retry");
            return 0;
        }

        Logger::info("Found " + std::to_string(files_to_retry.size()) + " files in transcoding error state (3) to retry");

        size_t retry_count = 0;
        for (const auto &file_path : files_to_retry)
        {
            // Check if this is a RAW file that needs transcoding
            if (isRawFile(file_path))
            {
                Logger::info("Retrying transcoding for file in error state: " + file_path);

                // Reset the flag to -1 (in progress) to allow retry
                auto &config_manager = PocoConfigAdapter::getInstance();
                bool pre_process_quality_stack = config_manager.getPreProcessQualityStack();

                std::vector<DedupMode> modes_to_reset;
                if (pre_process_quality_stack)
                {
                    modes_to_reset = {DedupMode::FAST, DedupMode::BALANCED, DedupMode::QUALITY};
                }
                else
                {
                    DedupMode current_mode = config_manager.getDedupMode();
                    modes_to_reset = {current_mode};
                }

                bool reset_success = true;
                for (const auto &mode : modes_to_reset)
                {
                    auto flag_result = db_manager_->resetProcessingFlag(file_path, mode);
                    if (!flag_result.success)
                    {
                        Logger::warn("Failed to reset processing flag for retry: " + file_path +
                                     " mode: " + DedupModes::getModeName(mode) + " - " + flag_result.error_message);
                        reset_success = false;
                    }
                }

                if (reset_success)
                {
                    // Queue the file for transcoding retry
                    queueForTranscoding(file_path);
                    retry_count++;
                    Logger::debug("Queued file for transcoding retry: " + file_path);
                }
            }
        }

        Logger::info("Retry operation completed. " + std::to_string(retry_count) + " files queued for transcoding retry");
        return retry_count;
    }
    catch (const std::exception &e)
    {
        Logger::error("Exception during retry operation: " + std::string(e.what()));
        return 0;
    }
}