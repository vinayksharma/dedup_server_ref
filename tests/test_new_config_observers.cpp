#include "gtest/gtest.h"
#include "poco_config_adapter.hpp"
#include "core/cache_config_observer.hpp"
#include "core/processing_config_observer.hpp"
#include "core/dedup_mode_config_observer.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>

using json = nlohmann::json;

class NewConfigObserversTest : public ::testing::Test
{
protected:
    std::string test_config_path_;

    void SetUp() override
    {
        // Create a test configuration file
        test_config_path_ = "test_new_config_observers.json";
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
            "processing": {
                "batch_size": 200
            },
            "cache": {
                "decoder_cache_size_mb": 256
            }
        })";
        config_file.close();
    }
};

TEST_F(NewConfigObserversTest, TestCacheConfigObserver)
{
    auto &config = PocoConfigAdapter::getInstance();

    // Create and subscribe cache config observer
    auto cache_observer = std::make_unique<CacheConfigObserver>();
    config.subscribe(cache_observer.get());

    // Test cache configuration change
    json update_config = {
        {"cache", {{"decoder_cache_size_mb", 512}}}};

    // This should trigger the cache config observer
    EXPECT_NO_THROW(config.updateConfig(update_config.dump()));

    // Verify the change was applied
    EXPECT_EQ(config.getDecoderCacheSizeMB(), 512);

    // Unsubscribe observer
    config.unsubscribe(cache_observer.get());
}

TEST_F(NewConfigObserversTest, TestProcessingConfigObserver)
{
    auto &config = PocoConfigAdapter::getInstance();

    // Create and subscribe processing config observer
    auto processing_observer = std::make_unique<ProcessingConfigObserver>();
    config.subscribe(processing_observer.get());

    // Test processing configuration change
    json update_config = {
        {"processing", {{"batch_size", 500}}},
        {"pre_process_quality_stack", false}};

    // This should trigger the processing config observer
    EXPECT_NO_THROW(config.updateConfig(update_config.dump()));

    // Verify the changes were applied
    EXPECT_EQ(config.getProcessingBatchSize(), 500);
    EXPECT_EQ(config.getPreProcessQualityStack(), false);

    // Unsubscribe observer
    config.unsubscribe(processing_observer.get());
}

TEST_F(NewConfigObserversTest, TestDedupModeConfigObserver)
{
    auto &config = PocoConfigAdapter::getInstance();

    // Create and subscribe dedup mode config observer
    auto dedup_mode_observer = std::make_unique<DedupModeConfigObserver>();
    config.subscribe(dedup_mode_observer.get());

    // Test dedup mode configuration change
    json update_config = {
        {"dedup_mode", "QUALITY"}};

    // This should trigger the dedup mode config observer
    EXPECT_NO_THROW(config.updateConfig(update_config.dump()));

    // Verify the change was applied
    EXPECT_EQ(config.getDedupMode(), DedupMode::QUALITY);

    // Unsubscribe observer
    config.unsubscribe(dedup_mode_observer.get());
}

TEST_F(NewConfigObserversTest, TestAllNewObserversTogether)
{
    auto &config = PocoConfigAdapter::getInstance();

    // Create and subscribe all new observers
    auto cache_observer = std::make_unique<CacheConfigObserver>();
    auto processing_observer = std::make_unique<ProcessingConfigObserver>();
    auto dedup_mode_observer = std::make_unique<DedupModeConfigObserver>();

    config.subscribe(cache_observer.get());
    config.subscribe(processing_observer.get());
    config.subscribe(dedup_mode_observer.get());

    // Test multiple configuration changes at once
    json update_config = {
        {"cache", {{"decoder_cache_size_mb", 1024}}},
        {"processing", {{"batch_size", 1000}}},
        {"pre_process_quality_stack", true},
        {"dedup_mode", "BALANCED"}};

    // This should trigger all observers
    EXPECT_NO_THROW(config.updateConfig(update_config.dump()));

    // Verify all changes were applied
    EXPECT_EQ(config.getDecoderCacheSizeMB(), 1024);
    EXPECT_EQ(config.getProcessingBatchSize(), 1000);
    EXPECT_EQ(config.getPreProcessQualityStack(), true);
    EXPECT_EQ(config.getDedupMode(), DedupMode::BALANCED);

    // Unsubscribe all observers
    config.unsubscribe(cache_observer.get());
    config.unsubscribe(processing_observer.get());
    config.unsubscribe(dedup_mode_observer.get());
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
