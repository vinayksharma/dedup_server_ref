#pragma once

#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <string>
#include <vector>
#include "config_observer.hpp"
#include "core/shutdown_manager.hpp"
#include "config_observer.hpp"

// Forward declarations
class FileProcessingEvent;
class DatabaseManager;
class PocoConfigAdapter;

/**
 * @brief Simple continuous processing manager that runs a single thread
 * to continuously process files without TBB or complex thread pool management
 *
 * This replaces the ThreadPoolManager with a much simpler approach:
 * - Single long-running processing thread
 * - Continuous database querying for work
 * - Simple sequential file processing
 * - No TBB dependencies
 */
class ContinuousProcessingManager : public ConfigObserver
{
public:
    /**
     * @brief Get singleton instance
     */
    static ContinuousProcessingManager &getInstance();

    /**
     * @brief Start the continuous processing thread
     */
    void start();

    /**
     * @brief Stop the continuous processing thread
     */
    void stop();

    /**
     * @brief Check if processing is currently running
     */
    bool isRunning() const;

    /**
     * @brief Set callback for processing events
     */
    void setProcessingCallback(std::function<void(const FileProcessingEvent &)> callback);

    /**
     * @brief Set callback for processing errors
     */
    void setErrorCallback(std::function<void(const std::exception &)> callback);

    /**
     * @brief Set callback for processing completion
     */
    void setCompletionCallback(std::function<void()> callback);

    /**
     * @brief Configuration change handler
     */
    void onConfigUpdate(const ConfigUpdateEvent &event) override;

    /**
     * @brief Destructor
     */
    ~ContinuousProcessingManager();

private:
    /**
     * @brief Private constructor for singleton
     */
    ContinuousProcessingManager();

    /**
     * @brief Main processing loop that runs in the processing thread
     */
    void processingLoop();

    /**
     * @brief Process a single file
     */
    void processSingleFile(const std::string &file_path, const std::string &file_name);

    /**
     * @brief Get current configuration values
     */
    void updateConfiguration();

    // Singleton instance
    static ContinuousProcessingManager *instance_;

    // Thread management
    std::thread processing_thread_;
    std::atomic<bool> running_{false};
    std::atomic<bool> shutdown_requested_{false};

    // Configuration
    std::atomic<int> batch_size_{50};
    std::atomic<int> idle_interval_seconds_{30};
    std::atomic<bool> pre_process_quality_stack_{false};
    std::atomic<int> dedup_mode_{0}; // 0=FAST, 1=BALANCED, 2=QUALITY

    // Callbacks
    std::function<void(const FileProcessingEvent &)> processing_callback_;
    std::function<void(const std::exception &)> error_callback_;
    std::function<void()> completion_callback_;

    // Thread safety
    mutable std::mutex config_mutex_;
    std::condition_variable shutdown_cv_;

    // Configuration observer management
    bool subscribed_to_config_{false};
};
