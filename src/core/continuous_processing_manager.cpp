#include "core/continuous_processing_manager.hpp"
#include "database/database_manager.hpp"
#include "poco_config_adapter.hpp"
#include "logging/logger.hpp"
#include "core/shutdown_manager.hpp"
#include "media_processing_orchestrator.hpp"
#include <chrono>

// Singleton instance
ContinuousProcessingManager *ContinuousProcessingManager::instance_ = nullptr;

ContinuousProcessingManager::ContinuousProcessingManager()
    : processing_callback_(nullptr), error_callback_(nullptr), completion_callback_(nullptr)
{
    Logger::info("ContinuousProcessingManager constructor called");

    // Subscribe to configuration changes
    try
    {
        auto &config_manager = PocoConfigAdapter::getInstance();
        config_manager.subscribe(this);
        subscribed_to_config_ = true;
        Logger::info("ContinuousProcessingManager subscribed to configuration changes");
    }
    catch (const std::exception &e)
    {
        Logger::warn("Failed to subscribe to configuration changes: " + std::string(e.what()));
    }

    // Load initial configuration
    updateConfiguration();
}

ContinuousProcessingManager::~ContinuousProcessingManager()
{
    stop();
    Logger::info("ContinuousProcessingManager destructor called");
}

ContinuousProcessingManager &ContinuousProcessingManager::getInstance()
{
    if (!instance_)
    {
        instance_ = new ContinuousProcessingManager();
    }
    return *instance_;
}

void ContinuousProcessingManager::start()
{
    if (running_.load())
    {
        Logger::warn("ContinuousProcessingManager is already running");
        return;
    }

    Logger::info("Starting ContinuousProcessingManager");
    running_.store(true);
    shutdown_requested_.store(false);

    // Start the processing thread
    processing_thread_ = std::thread(&ContinuousProcessingManager::processingLoop, this);

    Logger::info("ContinuousProcessingManager started successfully");
}

void ContinuousProcessingManager::stop()
{
    if (!running_.load())
    {
        return;
    }

    Logger::info("Stopping ContinuousProcessingManager");

    // Signal shutdown
    shutdown_requested_.store(true);
    running_.store(false);
    shutdown_cv_.notify_all();

    // Wait for thread to complete
    if (processing_thread_.joinable())
    {
        processing_thread_.join();
    }

    Logger::info("ContinuousProcessingManager stopped");
}

bool ContinuousProcessingManager::isRunning() const
{
    return running_.load();
}

void ContinuousProcessingManager::setProcessingCallback(std::function<void(const FileProcessingEvent &)> callback)
{
    processing_callback_ = callback;
}

void ContinuousProcessingManager::setErrorCallback(std::function<void(const std::exception &)> callback)
{
    error_callback_ = callback;
}

void ContinuousProcessingManager::setCompletionCallback(std::function<void()> callback)
{
    completion_callback_ = callback;
}

void ContinuousProcessingManager::onConfigUpdate(const ConfigUpdateEvent &event)
{
    Logger::debug("ContinuousProcessingManager received config update with " + std::to_string(event.changed_keys.size()) + " changed keys");

    for (const auto &key : event.changed_keys)
    {
        if (key == "processing_batch_size")
        {
            try
            {
                auto &config_manager = PocoConfigAdapter::getInstance();
                int new_batch_size = config_manager.getProcessingBatchSize();
                batch_size_.store(new_batch_size);
                Logger::info("Processing batch size updated to: " + std::to_string(new_batch_size));
            }
            catch (const std::exception &e)
            {
                Logger::warn("Failed to get processing_batch_size: " + std::string(e.what()));
            }
        }
        else if (key == "processing_interval_seconds")
        {
            try
            {
                auto &config_manager = PocoConfigAdapter::getInstance();
                int new_interval = config_manager.getProcessingIntervalSeconds();
                idle_interval_seconds_.store(new_interval);
                Logger::info("Processing idle interval updated to: " + std::to_string(new_interval) + " seconds");
            }
            catch (const std::exception &e)
            {
                Logger::warn("Failed to get processing_interval_seconds: " + std::string(e.what()));
            }
        }
        else if (key == "pre_process_quality_stack")
        {
            try
            {
                auto &config_manager = PocoConfigAdapter::getInstance();
                bool new_value = config_manager.getPreProcessQualityStack();
                pre_process_quality_stack_.store(new_value);
                Logger::info("Pre-process quality stack updated to: " + std::string(new_value ? "enabled" : "disabled"));
            }
            catch (const std::exception &e)
            {
                Logger::warn("Failed to get pre_process_quality_stack: " + std::string(e.what()));
            }
        }
        else if (key == "dedup_mode")
        {
            try
            {
                auto &config_manager = PocoConfigAdapter::getInstance();
                DedupMode mode = config_manager.getDedupMode();
                int new_mode = 0; // Default to FAST
                if (mode == DedupMode::FAST)
                {
                    new_mode = 0;
                }
                else if (mode == DedupMode::BALANCED)
                {
                    new_mode = 1;
                }
                else if (mode == DedupMode::QUALITY)
                {
                    new_mode = 2;
                }
                dedup_mode_.store(new_mode);
                Logger::info("Dedup mode updated to: " + std::to_string(new_mode));
            }
            catch (const std::exception &e)
            {
                Logger::warn("Failed to get dedup_mode: " + std::string(e.what()));
            }
        }
    }
}

void ContinuousProcessingManager::updateConfiguration()
{
    try
    {
        auto &config_manager = PocoConfigAdapter::getInstance();

        batch_size_.store(config_manager.getProcessingBatchSize());
        idle_interval_seconds_.store(config_manager.getProcessingIntervalSeconds());
        pre_process_quality_stack_.store(config_manager.getPreProcessQualityStack());

        // Convert dedup mode enum to integer
        DedupMode mode = config_manager.getDedupMode();
        if (mode == DedupMode::FAST)
        {
            dedup_mode_.store(0);
        }
        else if (mode == DedupMode::BALANCED)
        {
            dedup_mode_.store(1);
        }
        else if (mode == DedupMode::QUALITY)
        {
            dedup_mode_.store(2);
        }
        else
        {
            dedup_mode_.store(0); // Default to FAST
        }

        Logger::info("Configuration loaded - Batch size: " + std::to_string(batch_size_.load()) +
                     ", Idle interval: " + std::to_string(idle_interval_seconds_.load()) + "s" +
                     ", Quality stack: " + std::string(pre_process_quality_stack_.load() ? "enabled" : "disabled") +
                     ", Mode: " + std::to_string(dedup_mode_.load()));
    }
    catch (const std::exception &e)
    {
        Logger::warn("Failed to load configuration: " + std::string(e.what()));
    }
}

void ContinuousProcessingManager::processingLoop()
{
    Logger::info("Continuous processing loop started");

    while (running_.load() && !ShutdownManager::getInstance().isShutdownRequested())
    {
        try
        {
            // Get current configuration values
            int current_batch_size = batch_size_.load();
            int current_idle_interval = idle_interval_seconds_.load();
            bool current_pre_process_quality_stack = pre_process_quality_stack_.load();
            int current_dedup_mode = dedup_mode_.load();

            // Get database manager instance
            auto &db_manager = DatabaseManager::getInstance();

            // Get files that need processing
            std::vector<std::pair<std::string, std::string>> files_to_process;

            if (current_pre_process_quality_stack)
            {
                // Process all quality levels
                files_to_process = db_manager.getAndMarkFilesForProcessingAnyModeWithPriority(current_batch_size);
            }
            else
            {
                // Process only the selected mode
                DedupMode mode;
                switch (current_dedup_mode)
                {
                case 0:
                    mode = DedupMode::FAST;
                    break;
                case 1:
                    mode = DedupMode::BALANCED;
                    break;
                case 2:
                    mode = DedupMode::QUALITY;
                    break;
                default:
                    mode = DedupMode::FAST;
                    break;
                }
                files_to_process = db_manager.getAndMarkFilesForProcessingWithPriority(mode, current_batch_size);
            }

            if (files_to_process.empty())
            {
                // No work available - wait for configured interval
                Logger::debug("No files need processing, waiting " + std::to_string(current_idle_interval) + " seconds");

                // Wait with shutdown checking
                for (int i = 0; i < current_idle_interval && running_.load() && !ShutdownManager::getInstance().isShutdownRequested(); ++i)
                {
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }
                continue;
            }

            // Process the batch of files
            Logger::info("Processing batch of " + std::to_string(files_to_process.size()) + " files");

            size_t processed_count = 0;
            size_t successful_count = 0;
            size_t failed_count = 0;

            for (const auto &file_info : files_to_process)
            {
                if (!running_.load() || ShutdownManager::getInstance().isShutdownRequested())
                {
                    break;
                }

                try
                {
                    processSingleFile(file_info.first, file_info.second);
                    successful_count++;

                    if (processing_callback_)
                    {
                        // Create a simple success event
                        FileProcessingEvent event;
                        event.file_path = file_info.first;
                        event.success = true;
                        event.artifact_format = "processed";
                        event.artifact_confidence = 1.0;
                        processing_callback_(event);
                    }
                }
                catch (const std::exception &e)
                {
                    failed_count++;
                    Logger::error("Failed to process file " + file_info.first + ": " + std::string(e.what()));

                    if (error_callback_)
                    {
                        error_callback_(e);
                    }
                }

                processed_count++;
            }

            Logger::info("Batch completed - Processed: " + std::to_string(processed_count) +
                         ", Successful: " + std::to_string(successful_count) +
                         ", Failed: " + std::to_string(failed_count));

            // Call completion callback if set
            if (completion_callback_)
            {
                completion_callback_();
            }

            // Immediately look for more work (no waiting)
            // This ensures continuous processing when work is available
        }
        catch (const std::exception &e)
        {
            Logger::error("Error in processing loop: " + std::string(e.what()));

            if (error_callback_)
            {
                error_callback_(e);
            }

            // Wait a bit before retrying to avoid tight error loops
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }

    Logger::info("Continuous processing loop ended");
}

void ContinuousProcessingManager::processSingleFile(const std::string &file_path, const std::string &file_name)
{
    Logger::debug("Processing single file: " + file_path);

    // For now, we'll use a simple approach - just mark the file as processed
    // In a real implementation, you'd call your actual file processing logic here

    auto &db_manager = DatabaseManager::getInstance();

    // Get current dedup mode
    int current_dedup_mode = dedup_mode_.load();
    bool current_pre_process_quality_stack = pre_process_quality_stack_.load();

    if (current_pre_process_quality_stack)
    {
        // Process for all modes
        db_manager.setProcessingFlag(file_path, DedupMode::FAST);
        db_manager.setProcessingFlag(file_path, DedupMode::BALANCED);
        db_manager.setProcessingFlag(file_path, DedupMode::QUALITY);
    }
    else
    {
        // Process only for the selected mode
        DedupMode mode;
        switch (current_dedup_mode)
        {
        case 0:
            mode = DedupMode::FAST;
            break;
        case 1:
            mode = DedupMode::BALANCED;
            break;
        case 2:
            mode = DedupMode::QUALITY;
            break;
        default:
            mode = DedupMode::FAST;
            break;
        }
        db_manager.setProcessingFlag(file_path, mode);
    }

    Logger::debug("File processed successfully: " + file_path);
}
