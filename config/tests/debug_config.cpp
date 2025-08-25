#include "poco_config_adapter.hpp"
#include "poco_config_manager.hpp"
#include <iostream>
#include <iomanip>

void debugConfiguration()
{
    std::cout << "=== Configuration Debug Information ===" << std::endl;

    try
    {
        auto &config = PocoConfigAdapter::getInstance();

        std::cout << "\n1. Basic Configuration:" << std::endl;
        std::cout << "  Dedup Mode: " << DedupModes::getModeName(config.getDedupMode()) << std::endl;
        std::cout << "  Log Level: " << config.getLogLevel() << std::endl;
        std::cout << "  Server Port: " << config.getServerPort() << std::endl;
        std::cout << "  Server Host: " << config.getServerHost() << std::endl;
        std::cout << "  Scan Interval: " << config.getScanIntervalSeconds() << " seconds" << std::endl;

        std::cout << "\n2. Threading Configuration:" << std::endl;
        std::cout << "  Max Processing Threads: " << config.getMaxProcessingThreads() << std::endl;
        std::cout << "  Max Scan Threads: " << config.getMaxScanThreads() << std::endl;
        std::cout << "  Database Threads: " << config.getDatabaseThreads() << std::endl;
        std::cout << "  HTTP Server Threads: " << config.getHttpServerThreads() << std::endl;

        std::cout << "\n3. Processing Configuration:" << std::endl;
        std::cout << "  Processing Batch Size: " << config.getProcessingBatchSize() << std::endl;
        std::cout << "  Pre-process Quality Stack: " << (config.getPreProcessQualityStack() ? "Yes" : "No") << std::endl;

        std::cout << "\n4. File Type Support:" << std::endl;
        auto supported_types = config.getSupportedFileTypes();
        auto transcoding_types = config.getTranscodingFileTypes();

        std::cout << "  Supported File Types: " << supported_types.size() << " types" << std::endl;
        std::cout << "  Transcoding File Types: " << transcoding_types.size() << " types" << std::endl;

        // Show some examples
        int count = 0;
        std::cout << "  Sample supported types: ";
        for (const auto &[ext, enabled] : supported_types)
        {
            if (count++ < 10)
            {
                std::cout << ext << "(" << (enabled ? "✓" : "✗") << ") ";
            }
            else
            {
                std::cout << "...";
                break;
            }
        }
        std::cout << std::endl;

        std::cout << "\n5. Video Processing Configuration:" << std::endl;
        std::cout << "  QUALITY mode:" << std::endl;
        std::cout << "    Skip Duration: " << config.getVideoSkipDurationSeconds(DedupMode::QUALITY) << "s" << std::endl;
        std::cout << "    Frames Per Skip: " << config.getVideoFramesPerSkip(DedupMode::QUALITY) << std::endl;
        std::cout << "    Skip Count: " << config.getVideoSkipCount(DedupMode::QUALITY) << std::endl;

        std::cout << "  BALANCED mode:" << std::endl;
        std::cout << "    Skip Duration: " << config.getVideoSkipDurationSeconds(DedupMode::BALANCED) << "s" << std::endl;
        std::cout << "    Frames Per Skip: " << config.getVideoFramesPerSkip(DedupMode::BALANCED) << std::endl;
        std::cout << "    Skip Count: " << config.getVideoSkipCount(DedupMode::BALANCED) << std::endl;

        std::cout << "  FAST mode:" << std::endl;
        std::cout << "    Skip Duration: " << config.getVideoSkipDurationSeconds(DedupMode::FAST) << "s" << std::endl;
        std::cout << "    Frames Per Skip: " << config.getVideoFramesPerSkip(DedupMode::FAST) << std::endl;
        std::cout << "    Skip Count: " << config.getVideoSkipCount(DedupMode::FAST) << std::endl;

        std::cout << "\n6. Database Configuration:" << std::endl;
        std::cout << "  Max Retries: " << config.getDatabaseMaxRetries() << std::endl;
        std::cout << "  Backoff Base: " << config.getDatabaseBackoffBaseMs() << "ms" << std::endl;
        std::cout << "  Max Backoff: " << config.getDatabaseMaxBackoffMs() << "ms" << std::endl;
        std::cout << "  Busy Timeout: " << config.getDatabaseBusyTimeoutMs() << "ms" << std::endl;
        std::cout << "  Operation Timeout: " << config.getDatabaseOperationTimeoutMs() << "ms" << std::endl;

        std::cout << "\n7. Cache Configuration:" << std::endl;
        std::cout << "  Decoder Cache Size: " << config.getDecoderCacheSizeMB() << "MB" << std::endl;
        std::cout << "  Max Decoder Threads: " << config.getMaxDecoderThreads() << std::endl;

        std::cout << "\n=== Configuration Debug Complete ===" << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "❌ Configuration debug failed: " << e.what() << std::endl;
        throw;
    }
}

int main()
{
    try
    {
        debugConfiguration();
        std::cout << "✅ Configuration debug completed successfully!" << std::endl;
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "❌ Configuration debug failed: " << e.what() << std::endl;
        return 1;
    }
}