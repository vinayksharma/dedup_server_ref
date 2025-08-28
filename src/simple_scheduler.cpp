#include "core/simple_scheduler.hpp"
#include "logging/logger.hpp"
#include "poco_config_adapter.hpp"
#include <algorithm>

SimpleScheduler::SimpleScheduler()
{
    Logger::info("SimpleScheduler constructor called");

    // Initialize with current config values
    try
    {
        auto &config_manager = PocoConfigAdapter::getInstance();
        current_scan_interval_.store(config_manager.getScanIntervalSeconds());
        current_processing_interval_.store(config_manager.getProcessingIntervalSeconds());
        Logger::info("SimpleScheduler initialized with scan interval: " + std::to_string(current_scan_interval_.load()) +
                     "s, processing interval: " + std::to_string(current_processing_interval_.load()) + "s");
    }
    catch (const std::exception &e)
    {
        Logger::warn("SimpleScheduler: Could not initialize with config values, using defaults: " + std::string(e.what()));
    }
}

SimpleScheduler::~SimpleScheduler()
{
    stop();
    Logger::info("SimpleScheduler destructor called");
}

SimpleScheduler &SimpleScheduler::getInstance()
{
    static SimpleScheduler instance;
    return instance;
}

void SimpleScheduler::start()
{
    if (running_.load())
    {
        Logger::warn("SimpleScheduler is already running");
        return;
    }

    running_.store(true);
    scheduler_thread_ = std::thread(&SimpleScheduler::schedulerLoop, this);
    Logger::info("SimpleScheduler started");
}

void SimpleScheduler::stop()
{
    if (!running_.load())
    {
        return;
    }

    running_.store(false);

    if (scheduler_thread_.joinable())
    {
        scheduler_thread_.join();
    }

    Logger::info("SimpleScheduler stopped");
}

bool SimpleScheduler::isRunning() const
{
    return running_.load();
}

void SimpleScheduler::setScanCallback(std::function<void()> callback)
{
    scan_callback_ = callback;
}

void SimpleScheduler::setProcessingCallback(std::function<void()> callback)
{
    processing_callback_ = callback;
}

void SimpleScheduler::onConfigUpdate(const ConfigUpdateEvent &event)
{
    // Check for scan interval changes
    if (std::find(event.changed_keys.begin(), event.changed_keys.end(), "scan_interval_seconds") != event.changed_keys.end())
    {
        try
        {
            auto &config_manager = PocoConfigAdapter::getInstance();
            int new_interval = config_manager.getScanIntervalSeconds();
            handleScanIntervalChange(new_interval);
        }
        catch (const std::exception &e)
        {
            Logger::error("SimpleScheduler: Error handling scan interval configuration change: " + std::string(e.what()));
        }
    }

    // Check for processing interval changes
    if (std::find(event.changed_keys.begin(), event.changed_keys.end(), "processing_interval_seconds") != event.changed_keys.end())
    {
        try
        {
            auto &config_manager = PocoConfigAdapter::getInstance();
            int new_interval = config_manager.getProcessingIntervalSeconds();
            handleProcessingIntervalChange(new_interval);
        }
        catch (const std::exception &e)
        {
            Logger::error("SimpleScheduler: Error handling processing interval configuration change: " + std::string(e.what()));
        }
    }
}

void SimpleScheduler::handleScanIntervalChange(int new_interval)
{
    int old_interval = current_scan_interval_.load();
    current_scan_interval_.store(new_interval);

    Logger::info("SimpleScheduler: Scan interval changed from " + std::to_string(old_interval) +
                 "s to " + std::to_string(new_interval) + "s");

    // Reset the last scan time to allow immediate execution if the new interval is shorter
    if (new_interval < old_interval)
    {
        last_scan_time_ = std::chrono::system_clock::now() - std::chrono::seconds(old_interval);
        Logger::info("SimpleScheduler: Reset scan timer to allow immediate execution with new shorter interval");
    }
}

void SimpleScheduler::handleProcessingIntervalChange(int new_interval)
{
    int old_interval = current_processing_interval_.load();
    current_processing_interval_.store(new_interval);

    Logger::info("SimpleScheduler: Processing interval changed from " + std::to_string(old_interval) +
                 "s to " + std::to_string(new_interval) + "s");

    // Reset the last processing time to allow immediate execution if the new interval is shorter
    if (new_interval < old_interval)
    {
        last_processing_time_ = std::chrono::system_clock::now() - std::chrono::seconds(old_interval);
        Logger::info("SimpleScheduler: Reset processing timer to allow immediate execution with new shorter interval");
    }
}

void SimpleScheduler::schedulerLoop()
{
    Logger::info("SimpleScheduler loop started");

    while (running_.load())
    {
        auto now = std::chrono::system_clock::now();

        // Use cached scan interval for better performance
        int scan_interval = current_scan_interval_.load();
        auto time_since_last_scan = std::chrono::duration_cast<std::chrono::seconds>(
                                        now - last_scan_time_)
                                        .count();

        if (time_since_last_scan >= scan_interval && scan_callback_)
        {
            Logger::info("Executing scheduled scan (interval: " + std::to_string(scan_interval) + "s)");
            try
            {
                scan_callback_();
                last_scan_time_ = now;
                Logger::info("Scheduled scan completed successfully");
            }
            catch (const std::exception &e)
            {
                Logger::error("Error during scheduled scan: " + std::string(e.what()));
            }
        }

        // Use cached processing interval for better performance
        int processing_interval = current_processing_interval_.load();
        auto time_since_last_processing = std::chrono::duration_cast<std::chrono::seconds>(
                                              now - last_processing_time_)
                                              .count();

        if (time_since_last_processing >= processing_interval && processing_callback_)
        {
            Logger::info("Executing scheduled processing (interval: " + std::to_string(processing_interval) + "s)");
            try
            {
                processing_callback_();
                last_processing_time_ = now;
                Logger::info("Scheduled processing completed successfully");
            }
            catch (const std::exception &e)
            {
                Logger::error("Error during scheduled processing: " + std::string(e.what()));
            }
        }

        // Sleep for a short interval before checking again
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }

    Logger::info("SimpleScheduler loop ended");
}