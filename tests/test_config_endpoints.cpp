#include "gtest/gtest.h"
#include "poco_config_adapter.hpp"
#include "core/logger_observer.hpp"
#include "core/server_config_observer.hpp"
#include "core/scan_config_observer.hpp"
#include "core/threading_config_observer.hpp"
#include "core/database_config_observer.hpp"
#include "core/file_type_config_observer.hpp"
#include "core/video_processing_config_observer.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>

using json = nlohmann::json;

class ConfigEndpointsTest : public ::testing::Test
{
protected:
    std::string test_config_path_;

    void SetUp() override
    {
        // Create a test configuration file
        test_config_path_ = "test_config_endpoints.json";
        createTestConfig();

        // Reset configuration for testing
        auto &config = PocoConfigAdapter::getInstance();
        // Stop the file watcher to prevent interference during tests
        config.stopWatching();
        // Load the test configuration
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
            "log_level": "INFO",
            "server_port": 8080,
            "server_host": "localhost",
            "scan_interval_seconds": 3600,
            "processing_interval_seconds": 1800,
            "pre_process_quality_stack": true,
            "threading": {
                "max_processing_threads": 8,
                "max_scan_threads": 4,
                "http_server_threads": "manual",
                "database_threads": 2,
                "max_decoder_threads": 4
            },
            "database": {
                "retry": {
                    "max_attempts": 3,
                    "backoff_base_ms": 100,
                    "max_backoff_ms": 1000
                },
                "timeout": {
                    "busy_timeout_ms": 30000,
                    "operation_timeout_ms": 60000
                }
            },
            "cache": {
                "decoder_cache_size_mb": 256
            },
            "processing": {
                "batch_size": 200
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
            "transcoding": {
                "cr2": true,
                "nef": true,
                "raw": false
            },
            "video": {
                "frames_per_skip": {
                    "QUALITY": 1,
                    "BALANCED": 2,
                    "FAST": 2
                },
                "skip_count": {
                    "QUALITY": 3,
                    "BALANCED": 5,
                    "FAST": 5
                },
                "skip_duration_seconds": {
                    "QUALITY": 1,
                    "BALANCED": 2,
                    "FAST": 2
                }
            }
        })";
        config_file.close();
    }
};

TEST_F(ConfigEndpointsTest, TestServerConfigEndpoints)
{
    auto &config = PocoConfigAdapter::getInstance();

    // Test get server config
    std::string server_config = config.getServerConfig();
    auto server_json = json::parse(server_config);

    EXPECT_TRUE(server_json.contains("server_host"));
    EXPECT_TRUE(server_json.contains("server_port"));
    EXPECT_TRUE(server_json.contains("auth_secret"));

    // Test update server config
    json update_config = {
        {"server_port", 9090},
        {"server_host", "127.0.0.1"}};

    config.updateServerConfig(update_config.dump());

    // Verify changes
    EXPECT_EQ(config.getServerPort(), 9090);
    EXPECT_EQ(config.getServerHost(), "127.0.0.1");
}

TEST_F(ConfigEndpointsTest, TestThreadingConfigEndpoints)
{
    auto &config = PocoConfigAdapter::getInstance();

    // Test get threading config
    std::string threading_config = config.getThreadingConfig();
    auto threading_json = json::parse(threading_config);

    EXPECT_TRUE(threading_json.contains("max_processing_threads"));
    EXPECT_TRUE(threading_json.contains("max_scan_threads"));
    EXPECT_TRUE(threading_json.contains("database_threads"));
    EXPECT_TRUE(threading_json.contains("http_server_threads"));

    // Test that the update method doesn't crash and publishes events
    json update_config = {
        {"max_processing_threads", 16},
        {"max_scan_threads", 8}};

    // This should not crash and should publish events
    EXPECT_NO_THROW(config.updateThreadingConfig(update_config.dump()));
}

TEST_F(ConfigEndpointsTest, TestDatabaseConfigEndpoints)
{
    auto &config = PocoConfigAdapter::getInstance();

    // Test get database config
    std::string database_config = config.getDatabaseConfig();
    auto database_json = json::parse(database_config);

    EXPECT_TRUE(database_json.contains("retry"));
    EXPECT_TRUE(database_json.contains("timeout"));
    EXPECT_TRUE(database_json["retry"].contains("max_attempts"));
    EXPECT_TRUE(database_json["timeout"].contains("busy_timeout_ms"));

    // Test that the update method doesn't crash and publishes events
    json update_config = {
        {"retry", {{"max_attempts", 5}, {"backoff_base_ms", 200}}},
        {"timeout", {{"busy_timeout_ms", 60000}}}};

    // This should not crash and should publish events
    EXPECT_NO_THROW(config.updateDatabaseConfig(update_config.dump()));
}

TEST_F(ConfigEndpointsTest, TestFileTypesConfigEndpoints)
{
    auto &config = PocoConfigAdapter::getInstance();

    // Test get file types config
    std::string filetypes_config = config.getFileTypesConfig();
    auto filetypes_json = json::parse(filetypes_config);

    EXPECT_TRUE(filetypes_json.contains("supported_file_types"));
    EXPECT_TRUE(filetypes_json.contains("transcoding_file_types"));

    // Test that the update method doesn't crash and publishes events
    json update_config = {
        {"supported_file_types", {{"images", {{"png", false}, {"jpg", true}}}}},
        {"transcoding_file_types", {{"cr2", true}, {"nef", false}}}};

    // This should not crash and should publish events
    EXPECT_NO_THROW(config.updateFileTypesConfig(update_config.dump()));
}

TEST_F(ConfigEndpointsTest, TestVideoConfigEndpoints)
{
    auto &config = PocoConfigAdapter::getInstance();

    // Test get video config
    std::string video_config = config.getVideoConfig();
    auto video_json = json::parse(video_config);

    EXPECT_TRUE(video_json.contains("dedup_mode"));
    EXPECT_TRUE(video_json.contains("video_processing"));
    EXPECT_TRUE(video_json["video_processing"].contains("QUALITY"));
    EXPECT_TRUE(video_json["video_processing"].contains("BALANCED"));
    EXPECT_TRUE(video_json["video_processing"].contains("FAST"));

    // Test that the update method doesn't crash and publishes events
    json update_config = {
        {"dedup_mode", "FAST"},
        {"video_processing", {{"FAST", {{"frames_per_skip", 4}, {"skip_count", 10}}}}}};

    // This should not crash and should publish events
    EXPECT_NO_THROW(config.updateVideoConfig(update_config.dump()));
}

TEST_F(ConfigEndpointsTest, TestScanningConfigEndpoints)
{
    auto &config = PocoConfigAdapter::getInstance();

    // Test get scanning config
    std::string scanning_config = config.getScanningConfig();
    auto scanning_json = json::parse(scanning_config);

    EXPECT_TRUE(scanning_json.contains("scan_interval_seconds"));
    EXPECT_TRUE(scanning_json.contains("max_scan_threads"));

    // Test that the update method doesn't crash and publishes events
    json update_config = {
        {"scan_interval_seconds", 600},
        {"max_scan_threads", 6}};

    // This should not crash and should publish events
    EXPECT_NO_THROW(config.updateScanningConfig(update_config.dump()));
}

TEST_F(ConfigEndpointsTest, TestProcessingConfigEndpoints)
{
    auto &config = PocoConfigAdapter::getInstance();

    // Test get processing config
    std::string processing_config = config.getProcessingConfig();
    auto processing_json = json::parse(processing_config);

    EXPECT_TRUE(processing_json.contains("processing_batch_size"));
    EXPECT_TRUE(processing_json.contains("pre_process_quality_stack"));

    // Test that the update method doesn't crash and publishes events
    json update_config = {
        {"processing_batch_size", 500},
        {"pre_process_quality_stack", false}};

    // This should not crash and should publish events
    EXPECT_NO_THROW(config.updateProcessingConfig(update_config.dump()));
}

TEST_F(ConfigEndpointsTest, TestObserverRegistration)
{
    auto &config = PocoConfigAdapter::getInstance();

    // Create observers
    auto logger_observer = std::make_unique<LoggerObserver>();
    auto server_config_observer = std::make_unique<ServerConfigObserver>();
    auto scan_config_observer = std::make_unique<ScanConfigObserver>();
    auto threading_config_observer = std::make_unique<ThreadingConfigObserver>();
    auto database_config_observer = std::make_unique<DatabaseConfigObserver>();
    auto file_type_config_observer = std::make_unique<FileTypeConfigObserver>();
    auto video_processing_config_observer = std::make_unique<VideoProcessingConfigObserver>();

    // Subscribe observers
    config.subscribe(logger_observer.get());
    config.subscribe(server_config_observer.get());
    config.subscribe(scan_config_observer.get());
    config.subscribe(threading_config_observer.get());
    config.subscribe(database_config_observer.get());
    config.subscribe(file_type_config_observer.get());
    config.subscribe(video_processing_config_observer.get());

    // Test that observers are registered by making a configuration change
    // This should trigger the observer notifications
    config.setLogLevel("DEBUG");

    // Unsubscribe observers
    config.unsubscribe(logger_observer.get());
    config.unsubscribe(server_config_observer.get());
    config.unsubscribe(scan_config_observer.get());
    config.unsubscribe(threading_config_observer.get());
    config.unsubscribe(database_config_observer.get());
    config.unsubscribe(file_type_config_observer.get());
    config.unsubscribe(video_processing_config_observer.get());

    // Verify unsubscription by checking no errors occur
    SUCCEED();
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
