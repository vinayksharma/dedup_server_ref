#include <gtest/gtest.h>
#include "core/poco_config_adapter.hpp"
#include "core/dedup_modes.hpp"
#include "logging/logger.hpp"
#include <fstream>
#include <filesystem>

// Include the new config observer interface
#include "core/config_observer.hpp"

class PocoConfigAdapterTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Initialize logger for tests
        Logger::init("DEBUG");

        // Create a temporary test config file
        test_config_path_ = "test_config.json";
        createTestConfig();

        // Reset the configuration to test state before each test
        auto &config = PocoConfigAdapter::getInstance();
        config.loadConfig(test_config_path_);
    }

    void TearDown() override
    {
        // Clean up test files
        if (std::filesystem::exists(test_config_path_))
        {
            std::filesystem::remove(test_config_path_);
        }
    }

    void createTestConfig()
    {
        std::ofstream config_file(test_config_path_);
        config_file << R"({
            "auth_secret": "test-secret-key",
            "dedup_mode": "FAST",
            "log_level": "DEBUG",
            "server_port": 9090,
            "server_host": "test-host",
            "scan_interval_seconds": 1800,
            "processing_interval_seconds": 900,
            "pre_process_quality_stack": true,
            "threading": {
                "max_processing_threads": 4,
                "max_scan_threads": 2,
                "http_server_threads": "manual",
                "database_threads": 1,
                "max_decoder_threads": 2
            },
            "database": {
                "retry": {
                    "max_attempts": 5,
                    "backoff_base_ms": 200,
                    "max_backoff_ms": 2000
                },
                "timeout": {
                    "busy_timeout_ms": 45000,
                    "operation_timeout_ms": 90000
                }
            },
            "cache": {
                "decoder_cache_size_mb": 512
            },
            "processing": {
                "batch_size": 50
            },
            "categories": {
                "images": {
                    "jpg": true,
                    "png": true,
                    "gif": false
                },
                "video": {
                    "mp4": true,
                    "avi": false,
                    "mov": true
                },
                "audio": {
                    "mp3": true,
                    "wav": false
                }
            }
        })";
        config_file.close();
    }

    std::string test_config_path_;
};

// Test basic functionality
TEST_F(PocoConfigAdapterTest, SingletonPattern)
{
    auto &instance1 = PocoConfigAdapter::getInstance();
    auto &instance2 = PocoConfigAdapter::getInstance();

    EXPECT_EQ(&instance1, &instance2);
}

// Test configuration getters - verify they delegate to PocoConfigManager
TEST_F(PocoConfigAdapterTest, ConfigurationGetters)
{
    auto &config = PocoConfigAdapter::getInstance();

    EXPECT_EQ(config.getDedupMode(), DedupMode::FAST);
    EXPECT_EQ(config.getLogLevel(), "DEBUG");
    EXPECT_EQ(config.getServerPort(), 9090);
    EXPECT_EQ(config.getServerHost(), "test-host");
    EXPECT_EQ(config.getAuthSecret(), "test-secret-key");
    EXPECT_EQ(config.getScanIntervalSeconds(), 1800);
    EXPECT_EQ(config.getProcessingIntervalSeconds(), 900);
    EXPECT_EQ(config.getPreProcessQualityStack(), true);
}

// Test thread configuration getters
TEST_F(PocoConfigAdapterTest, ThreadConfigurationGetters)
{
    auto &config = PocoConfigAdapter::getInstance();

    EXPECT_EQ(config.getMaxProcessingThreads(), 4);
    EXPECT_EQ(config.getMaxScanThreads(), 2);
    EXPECT_EQ(config.getHttpServerThreads(), "manual");
    EXPECT_EQ(config.getDatabaseThreads(), 1);
    EXPECT_EQ(config.getMaxDecoderThreads(), 2);
}

// Test database configuration getters
TEST_F(PocoConfigAdapterTest, DatabaseConfigurationGetters)
{
    auto &config = PocoConfigAdapter::getInstance();

    EXPECT_EQ(config.getDatabaseMaxRetries(), 5);
    EXPECT_EQ(config.getDatabaseBackoffBaseMs(), 200);
    EXPECT_EQ(config.getDatabaseMaxBackoffMs(), 2000);
    EXPECT_EQ(config.getDatabaseBusyTimeoutMs(), 45000);
    EXPECT_EQ(config.getDatabaseOperationTimeoutMs(), 90000);
}

// Test cache configuration getters
TEST_F(PocoConfigAdapterTest, CacheConfigurationGetters)
{
    auto &config = PocoConfigAdapter::getInstance();

    EXPECT_EQ(config.getDecoderCacheSizeMB(), 512);
}

// Test processing configuration getters
TEST_F(PocoConfigAdapterTest, ProcessingConfigurationGetters)
{
    auto &config = PocoConfigAdapter::getInstance();

    EXPECT_EQ(config.getProcessingBatchSize(), 50);
}

// Test file type configuration getters
TEST_F(PocoConfigAdapterTest, FileTypeConfigurationGetters)
{
    auto &config = PocoConfigAdapter::getInstance();

    auto supported_types = config.getSupportedFileTypes();
    EXPECT_TRUE(supported_types["jpg"]);
    EXPECT_TRUE(supported_types["png"]);
    EXPECT_FALSE(supported_types["gif"]);
    EXPECT_TRUE(supported_types["mp4"]);
    EXPECT_FALSE(supported_types["avi"]);
    EXPECT_TRUE(supported_types["mp3"]);
    EXPECT_FALSE(supported_types["wav"]);

    auto transcoding_types = config.getTranscodingFileTypes();
    EXPECT_TRUE(transcoding_types["mp4"]);
    EXPECT_FALSE(transcoding_types["avi"]);
    EXPECT_TRUE(transcoding_types["mov"]);
    EXPECT_TRUE(transcoding_types["mp3"]);
    EXPECT_FALSE(transcoding_types["wav"]);
}

// Test enabled extensions getters
TEST_F(PocoConfigAdapterTest, EnabledExtensionsGetters)
{
    auto &config = PocoConfigAdapter::getInstance();

    auto enabled_types = config.getEnabledFileTypes();
    EXPECT_EQ(enabled_types.size(), 5); // jpg, png, mp4, mov, mp3 (from test config)

    auto image_extensions = config.getEnabledImageExtensions();
    EXPECT_EQ(image_extensions.size(), 2);
    EXPECT_TRUE(std::find(image_extensions.begin(), image_extensions.end(), "jpg") != image_extensions.end());
    EXPECT_TRUE(std::find(image_extensions.begin(), image_extensions.end(), "png") != image_extensions.end());

    auto video_extensions = config.getEnabledVideoExtensions();
    EXPECT_EQ(video_extensions.size(), 2);
    EXPECT_TRUE(std::find(video_extensions.begin(), video_extensions.end(), "mp4") != video_extensions.end());
    EXPECT_TRUE(std::find(video_extensions.begin(), video_extensions.end(), "mov") != video_extensions.end());

    auto audio_extensions = config.getEnabledAudioExtensions();
    EXPECT_EQ(audio_extensions.size(), 1);
    EXPECT_TRUE(std::find(audio_extensions.begin(), audio_extensions.end(), "mp3") != audio_extensions.end());
}

// Test transcoding needs
TEST_F(PocoConfigAdapterTest, TranscodingNeeds)
{
    auto &config = PocoConfigAdapter::getInstance();

    EXPECT_TRUE(config.needsTranscoding("mp4"));
    EXPECT_FALSE(config.needsTranscoding("avi"));
    EXPECT_TRUE(config.needsTranscoding("mp3"));
    EXPECT_FALSE(config.needsTranscoding("wav"));
    EXPECT_FALSE(config.needsTranscoding("jpg"));
    EXPECT_FALSE(config.needsTranscoding("png"));
}

// Test configuration setters
TEST_F(PocoConfigAdapterTest, ConfigurationSetters)
{
    auto &config = PocoConfigAdapter::getInstance();

    // Test setting dedup mode
    config.setDedupMode(DedupMode::BALANCED);
    EXPECT_EQ(config.getDedupMode(), DedupMode::BALANCED);

    // Test setting log level
    config.setLogLevel("INFO");
    EXPECT_EQ(config.getLogLevel(), "INFO");

    // Test setting server port
    config.setServerPort(8080);
    EXPECT_EQ(config.getServerPort(), 8080);

    // Test setting auth secret
    config.setAuthSecret("new-secret");
    EXPECT_EQ(config.getAuthSecret(), "new-secret");
}

// Test updateConfig method
TEST_F(PocoConfigAdapterTest, UpdateConfig)
{
    auto &config = PocoConfigAdapter::getInstance();

    // Create a JSON string with updates
    std::string updates = R"({
        "server_port": 7070,
        "log_level": "WARN"
    })";

    // Update configuration
    config.updateConfig(updates);

    // Verify updates
    EXPECT_EQ(config.getServerPort(), 7070);
    EXPECT_EQ(config.getLogLevel(), "WARN");
}

// Test configuration sections
TEST_F(PocoConfigAdapterTest, ConfigurationSections)
{
    auto &config = PocoConfigAdapter::getInstance();

    auto processing_config = config.getProcessingConfig();
    // processing_config is now a JSON string, so we can't directly access fields
    // We can verify it's not empty and contains expected content
    EXPECT_FALSE(processing_config.empty());
    EXPECT_TRUE(processing_config.find("max_processing_threads") != std::string::npos);
    EXPECT_TRUE(processing_config.find("max_scan_threads") != std::string::npos);
    EXPECT_TRUE(processing_config.find("batch_size") != std::string::npos);

    auto cache_config = config.getCacheConfig();
    EXPECT_FALSE(cache_config.empty());
    EXPECT_TRUE(cache_config.find("decoder_cache_size_mb") != std::string::npos);
}

// Test configuration validation
TEST_F(PocoConfigAdapterTest, ConfigurationValidation)
{
    auto &config = PocoConfigAdapter::getInstance();

    // Test validation
    EXPECT_TRUE(config.validateConfig());
    EXPECT_TRUE(config.validateProcessingConfig());
    EXPECT_TRUE(config.validateCacheConfig());
}

// Test observer pattern
TEST_F(PocoConfigAdapterTest, ObserverPattern)
{
    auto &config = PocoConfigAdapter::getInstance();

    // Create a mock observer
    class MockObserver : public ConfigObserver
    {
    public:
        void onConfigUpdate(const ConfigUpdateEvent &event) override
        {
            last_event_ = event;
            event_count_++;
        }

        ConfigUpdateEvent last_event_;
        int event_count_ = 0;
    };

    MockObserver observer;

    // Subscribe observer
    config.subscribe(&observer);

    // Make a configuration change
    config.setDedupMode(DedupMode::QUALITY);

    // Verify event was published
    EXPECT_EQ(observer.event_count_, 1);
    EXPECT_EQ(observer.last_event_.source, "api");
    EXPECT_FALSE(observer.last_event_.changed_keys.empty());
    EXPECT_EQ(observer.last_event_.changed_keys[0], "dedup_mode");

    // Unsubscribe observer
    config.unsubscribe(&observer);

    // Make another change
    config.setLogLevel("ERROR");

    // Verify no more events
    EXPECT_EQ(observer.event_count_, 1);
}

// Test file watching
TEST_F(PocoConfigAdapterTest, FileWatching)
{
    auto &config = PocoConfigAdapter::getInstance();

    // Start watching
    config.startWatching(test_config_path_, 1);

    // Wait a bit for watcher to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Stop watching
    config.stopWatching();
}

// Test default values
TEST_F(PocoConfigAdapterTest, DefaultValues)
{
    auto &config = PocoConfigAdapter::getInstance();

    // Test values from the test configuration file
    EXPECT_EQ(config.getDedupMode(), DedupMode::FAST);
    EXPECT_EQ(config.getLogLevel(), "DEBUG");
    EXPECT_EQ(config.getServerPort(), 9090);
    EXPECT_EQ(config.getServerHost(), "test-host");
    EXPECT_EQ(config.getScanIntervalSeconds(), 1800);
    EXPECT_EQ(config.getProcessingIntervalSeconds(), 900);
    EXPECT_EQ(config.getMaxProcessingThreads(), 4);
    EXPECT_EQ(config.getMaxScanThreads(), 2);
    EXPECT_EQ(config.getDatabaseThreads(), 1);
    EXPECT_EQ(config.getMaxDecoderThreads(), 2);
    EXPECT_EQ(config.getProcessingBatchSize(), 50);
    EXPECT_EQ(config.getPreProcessQualityStack(), true);
    EXPECT_EQ(config.getDecoderCacheSizeMB(), 512);
}
