#pragma once

#include <string>
#include <chrono>
#include <thread>
#include <atomic>
#include <functional>
#include "logging/logger.hpp"
#include "config_observer.hpp"

/**
 * @brief Simple scheduler that uses config intervals for scan and processing operations
 *
 * This scheduler is observable and will automatically adjust scan and processing intervals
 * when configuration changes are detected.
 */
class SimpleScheduler : public ConfigObserver
{
public:
    // Singleton pattern
    static SimpleScheduler &getInstance();

    // Control
    void start();
    void stop();
    bool isRunning() const;

    // Configuration
    void setScanCallback(std::function<void()> callback);
    void setProcessingCallback(std::function<void()> callback);

    // ConfigObserver implementation
    void onConfigUpdate(const ConfigUpdateEvent &event) override;

private:
    SimpleScheduler();
    ~SimpleScheduler();
    SimpleScheduler(const SimpleScheduler &) = delete;
    SimpleScheduler &operator=(const SimpleScheduler &) = delete;

    // Internal methods
    void schedulerLoop();
    void handleScanIntervalChange(int new_interval);
    void handleProcessingIntervalChange(int new_interval);

    // Member variables
    std::atomic<bool> running_{false};
    std::thread scheduler_thread_;

    std::function<void()> scan_callback_;
    std::function<void()> processing_callback_;

    std::chrono::system_clock::time_point last_scan_time_;
    std::chrono::system_clock::time_point last_processing_time_;

    // Current intervals (cached for performance)
    std::atomic<int> current_scan_interval_{300};       // Default 5 minutes
    std::atomic<int> current_processing_interval_{600}; // Default 10 minutes
};