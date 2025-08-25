#include <iostream>
#include <thread>
#include <chrono>
#include "poco_config_adapter.hpp"
#include "core/dedup_modes.hpp"

/**
 * Example demonstrating configuration persistence functionality
 *
 * This example shows how to:
 * 1. Change configuration values programmatically
 * 2. Automatically persist changes to config.json
 * 3. Use bulk configuration updates
 * 4. Handle file type configurations
 */

int main()
{
    std::cout << "=== Configuration Persistence Example ===\n\n";

    auto &config = PocoConfigAdapter::getInstance();

    // Load configuration
    if (!config.loadConfig("config/config.json"))
    {
        std::cout << "Failed to load configuration\n";
        return 1;
    }

    std::cout << "Initial configuration:\n";
    std::cout << "  Dedup Mode: " << DedupModes::getModeName(config.getDedupMode()) << "\n";
    std::cout << "  Log Level: " << config.getLogLevel() << "\n";
    std::cout << "  Server Port: " << config.getServerPort() << "\n";
    std::cout << "  Max Processing Threads: " << config.getMaxProcessingThreads() << "\n\n";

    // Example 1: Change individual configuration values
    std::cout << "1. Changing individual configuration values...\n";

    config.setDedupMode(DedupMode::QUALITY);
    config.setLogLevel("DEBUG");
    config.setServerPort(8081);
    config.setMaxProcessingThreads(16);

    std::cout << "   ✓ Configuration values updated and persisted\n\n";

    // Example 2: Bulk configuration update
    std::cout << "2. Performing bulk configuration update...\n";

    nlohmann::json bulk_updates = {
        {"scan_interval_seconds", 120},
        {"processing_interval_seconds", 90},
        {"max_scan_threads", 8},
        {"pre_process_quality_stack", true}};

    config.updateConfigAndPersist(bulk_updates);

    std::cout << "   ✓ Bulk configuration update completed and persisted\n\n";

    // Example 3: Update file type configuration using specific setters
    std::cout << "\n=== File Type Configuration ===" << std::endl;

    // Enable/disable specific file types
    config.setFileTypeEnabled("images", "jpg", true);
    config.setFileTypeEnabled("images", "png", false);
    config.setFileTypeEnabled("raw", "cr2", true);
    config.setFileTypeEnabled("raw", "nef", false);

    // Set transcoding file types
    config.setTranscodingFileType("mp4", true);
    config.setTranscodingFileType("avi", false);

    std::cout << "File type configuration updated and persisted" << std::endl;

    // Example 4: Update video processing configuration using specific setters
    std::cout << "\n=== Video Processing Configuration ===" << std::endl;

    config.setVideoSkipDurationSeconds(5);
    config.setVideoFramesPerSkip(10);
    config.setVideoSkipCount(3);

    std::cout << "Video processing configuration updated and persisted" << std::endl;

    // Example 5: Update threading configuration using specific setters
    std::cout << "\n=== Threading Configuration ===" << std::endl;

    config.setMaxProcessingThreads(8);
    config.setMaxScanThreads(4);
    config.setHttpServerThreads(2);
    config.setDatabaseThreads(2);
    config.setMaxDecoderThreads(3);

    std::cout << "Threading configuration updated and persisted" << std::endl;

    // Example 6: Update processing configuration using specific setters
    std::cout << "\n=== Processing Configuration ===" << std::endl;

    config.setProcessingBatchSize(100);
    config.setScanIntervalSeconds(30);
    config.setProcessingIntervalSeconds(15);

    std::cout << "Processing configuration updated and persisted" << std::endl;

    // Example 7: Update database configuration using specific setters
    std::cout << "\n=== Database Configuration ===" << std::endl;

    config.setDatabaseMaxRetries(5);
    config.setDatabaseBackoffBaseMs(100);
    config.setDatabaseMaxBackoffMs(5000);
    config.setDatabaseBusyTimeoutMs(30000);
    config.setDatabaseOperationTimeoutMs(60000);

    std::cout << "Database configuration updated and persisted" << std::endl;

    // Example 8: Update decoder cache configuration using specific setters
    std::cout << "\n=== Decoder Cache Configuration ===" << std::endl;

    config.setDecoderCacheSizeMB(512);

    std::cout << "Decoder cache configuration updated and persisted" << std::endl;

    // Verify changes by reloading configuration
    std::cout << "6. Verifying changes by reloading configuration...\n";

    config.loadConfig("config/config.json");

    std::cout << "Updated configuration:\n";
    std::cout << "  Dedup Mode: " << DedupModes::getModeName(config.getDedupMode()) << "\n";
    std::cout << "  Log Level: " << config.getLogLevel() << "\n";
    std::cout << "  Server Port: " << config.getServerPort() << "\n";
    std::cout << "  Max Processing Threads: " << config.getMaxProcessingThreads() << "\n";
    std::cout << "  Scan Interval: " << config.getScanIntervalSeconds() << " seconds\n";
    std::cout << "  Processing Interval: " << config.getProcessingIntervalSeconds() << " seconds\n";
    std::cout << "  Max Scan Threads: " << config.getMaxScanThreads() << "\n";
    std::cout << "  Pre-process Quality Stack: " << (config.getPreProcessQualityStack() ? "enabled" : "disabled") << "\n";
    std::cout << "  Database Max Retries: " << config.getDatabaseMaxRetries() << "\n";
    std::cout << "  Database Busy Timeout: " << config.getDatabaseBusyTimeoutMs() << "ms\n\n";

    // Example 6: Cache configuration
    std::cout << "7. Updating cache configuration...\n";

    std::string cache_config = R"({
        "decoder_cache_size_mb": 512,
        "cache_cleanup_interval": 3600,
        "max_cache_age_hours": 24
    })";

    config.updateCacheConfig(cache_config);

    std::cout << "   ✓ Cache configuration updated and persisted\n\n";

    std::cout << "=== Configuration Persistence Example Completed ===\n";
    std::cout << "All changes have been automatically persisted to config/config.json\n";
    std::cout << "You can verify the changes by checking the file contents.\n";

    return 0;
}
