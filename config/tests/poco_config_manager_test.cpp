#include <gtest/gtest.h>
#include "poco_config_manager.hpp"
#include "core/dedup_modes.hpp"
#include "logging/logger.hpp"
#include <fstream>
#include <filesystem>

class PocoConfigManagerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Initialize logger for tests
        Logger::init("DEBUG");

        // Create a temporary test config file
        test_config_path_ = "test_config.json";
        createTestConfig();

        // Load the test configuration into the PocoConfigManager instance
        auto &config = PocoConfigManager::getInstance();
        config.load(test_config_path_);
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
            "cache_cleanup": {
                "fully_processed_age_days": 5,
                "partially_processed_age_days": 2,
                "unprocessed_age_days": 1,
                "require_all_modes": false,
                "cleanup_threshold_percent": 75
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
            },
            "video_processing": {
                "FAST": {
                    "skip_duration_seconds": 3,
                    "frames_per_skip": 3,
                    "skip_count": 6
                },
                "BALANCED": {
                    "skip_duration_seconds": 2,
                    "frames_per_skip": 2,
                    "skip_count": 10
                },
                "QUALITY": {
                    "skip_duration_seconds": 1,
                    "frames_per_skip": 4,
                    "skip_count": 15
                }
            }
        })";
        config_file.close();
    }

    std::string test_config_path_;
};

// Test basic functionality
TEST_F(PocoConfigManagerTest, SingletonPattern)
{
    auto &instance1 = PocoConfigManager::getInstance();
    auto &instance2 = PocoConfigManager::getInstance();

    EXPECT_EQ(&instance1, &instance2);
}

TEST_F(PocoConfigManagerTest, LoadAndSave)
{
    auto &config = PocoConfigManager::getInstance();

    // Test loading
    EXPECT_TRUE(config.load(test_config_path_));

    // Test saving to a new file
    std::string save_path = "test_save.json";
    EXPECT_TRUE(config.save(save_path));

    // Verify the saved file exists
    EXPECT_TRUE(std::filesystem::exists(save_path));

    // Clean up
    std::filesystem::remove(save_path);
}

// Test basic configuration getters
TEST_F(PocoConfigManagerTest, BasicConfigurationGetters)
{
    auto &config = PocoConfigManager::getInstance();
    EXPECT_TRUE(config.load(test_config_path_));

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
TEST_F(PocoConfigManagerTest, ThreadConfigurationGetters)
{
    auto &config = PocoConfigManager::getInstance();
    EXPECT_TRUE(config.load(test_config_path_));

    EXPECT_EQ(config.getMaxProcessingThreads(), 4);
    EXPECT_EQ(config.getMaxScanThreads(), 2);
    EXPECT_EQ(config.getHttpServerThreads(), "manual");
    EXPECT_EQ(config.getDatabaseThreads(), 1);
    EXPECT_EQ(config.getMaxDecoderThreads(), 2);
}

// Test database configuration getters
TEST_F(PocoConfigManagerTest, DatabaseConfigurationGetters)
{
    auto &config = PocoConfigManager::getInstance();
    EXPECT_TRUE(config.load(test_config_path_));

    EXPECT_EQ(config.getDatabaseMaxRetries(), 5);
    EXPECT_EQ(config.getDatabaseBackoffBaseMs(), 200);
    EXPECT_EQ(config.getDatabaseMaxBackoffMs(), 2000);
    EXPECT_EQ(config.getDatabaseBusyTimeoutMs(), 45000);
    EXPECT_EQ(config.getDatabaseOperationTimeoutMs(), 90000);
}

// Test cache configuration getters
TEST_F(PocoConfigManagerTest, CacheConfigurationGetters)
{
    auto &config = PocoConfigManager::getInstance();
    EXPECT_TRUE(config.load(test_config_path_));

    EXPECT_EQ(config.getDecoderCacheSizeMB(), 512);
}

// Test processing configuration getters
TEST_F(PocoConfigManagerTest, ProcessingConfigurationGetters)
{
    auto &config = PocoConfigManager::getInstance();
    EXPECT_TRUE(config.load(test_config_path_));

    EXPECT_EQ(config.getProcessingBatchSize(), 50);
}

// Test file type configuration getters
TEST_F(PocoConfigManagerTest, FileTypeConfigurationGetters)
{
    auto &config = PocoConfigManager::getInstance();
    EXPECT_TRUE(config.load(test_config_path_));

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
TEST_F(PocoConfigManagerTest, EnabledExtensionsGetters)
{
    auto &config = PocoConfigManager::getInstance();

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
TEST_F(PocoConfigManagerTest, TranscodingNeeds)
{
    auto &config = PocoConfigManager::getInstance();
    EXPECT_TRUE(config.load(test_config_path_));

    EXPECT_TRUE(config.needsTranscoding("mp4"));
    EXPECT_FALSE(config.needsTranscoding("avi"));
    EXPECT_TRUE(config.needsTranscoding("mp3"));
    EXPECT_FALSE(config.needsTranscoding("wav"));
    EXPECT_FALSE(config.needsTranscoding("jpg"));
    EXPECT_FALSE(config.needsTranscoding("png"));
}

// Test video processing configuration getters
TEST_F(PocoConfigManagerTest, VideoProcessingConfigurationGetters)
{
    auto &config = PocoConfigManager::getInstance();
    EXPECT_TRUE(config.load(test_config_path_));

    EXPECT_EQ(config.getVideoSkipDurationSeconds(DedupMode::FAST), 3);
    EXPECT_EQ(config.getVideoFramesPerSkip(DedupMode::FAST), 3);
    EXPECT_EQ(config.getVideoSkipCount(DedupMode::FAST), 6);

    EXPECT_EQ(config.getVideoSkipDurationSeconds(DedupMode::BALANCED), 2);
    EXPECT_EQ(config.getVideoFramesPerSkip(DedupMode::BALANCED), 2);
    EXPECT_EQ(config.getVideoSkipCount(DedupMode::BALANCED), 10);

    EXPECT_EQ(config.getVideoSkipDurationSeconds(DedupMode::QUALITY), 1);
    EXPECT_EQ(config.getVideoFramesPerSkip(DedupMode::QUALITY), 4);
    EXPECT_EQ(config.getVideoSkipCount(DedupMode::QUALITY), 15);
}

// Test configuration validation
TEST_F(PocoConfigManagerTest, ConfigurationValidation)
{
    auto &config = PocoConfigManager::getInstance();
    EXPECT_TRUE(config.load(test_config_path_));

    EXPECT_TRUE(config.validateConfig());
    EXPECT_TRUE(config.validateProcessingConfig());
    EXPECT_TRUE(config.validateCacheConfig());
}

// Test configuration sections
TEST_F(PocoConfigManagerTest, ConfigurationSections)
{
    auto &config = PocoConfigManager::getInstance();
    EXPECT_TRUE(config.load(test_config_path_));

    auto processing_config = config.getProcessingConfig();
    EXPECT_EQ(processing_config["max_processing_threads"], 4);
    EXPECT_EQ(processing_config["max_scan_threads"], 2);
    EXPECT_EQ(processing_config["batch_size"], 50);
    EXPECT_EQ(processing_config["dedup_mode"], "FAST");
    EXPECT_EQ(processing_config["pre_process_quality_stack"], true);

    auto cache_config = config.getCacheConfig();
    EXPECT_EQ(cache_config["decoder_cache_size_mb"], 512);
    EXPECT_EQ(cache_config["cache_cleanup"]["fully_processed_age_days"], 5);
    EXPECT_EQ(cache_config["cache_cleanup"]["cleanup_threshold_percent"], 75);
}

// Test update functionality
TEST_F(PocoConfigManagerTest, UpdateConfiguration)
{
    auto &config = PocoConfigManager::getInstance();
    EXPECT_TRUE(config.load(test_config_path_));

    // Update a value
    nlohmann::json update = {{"server_port", 8080}};
    config.update(update);

    EXPECT_EQ(config.getServerPort(), 8080);

    // Update nested value
    nlohmann::json nested_update = {{"threading", {{"max_processing_threads", 8}}}};
    config.update(nested_update);

    EXPECT_EQ(config.getMaxProcessingThreads(), 8);
}

// Test default values
TEST_F(PocoConfigManagerTest, DefaultValues)
{
    auto &config = PocoConfigManager::getInstance();

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

// Test utility methods
TEST_F(PocoConfigManagerTest, UtilityMethods)
{
    auto &config = PocoConfigManager::getInstance();
    EXPECT_TRUE(config.load(test_config_path_));

    EXPECT_TRUE(config.hasKey("dedup_mode"));
    EXPECT_TRUE(config.hasKey("threading.max_processing_threads"));
    EXPECT_FALSE(config.hasKey("nonexistent_key"));
}

// Test error handling
TEST_F(PocoConfigManagerTest, ErrorHandling)
{
    auto &config = PocoConfigManager::getInstance();

    // Test loading non-existent file
    EXPECT_FALSE(config.load("nonexistent_file.json"));

    // Test saving to invalid path
    EXPECT_FALSE(config.save("/invalid/path/config.json"));
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
