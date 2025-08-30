#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <atomic>
#include "core/decoder/media_decoder.hpp"
#include "core/transcoding_manager.hpp"
#include "poco_config_adapter.hpp"
#include "database/database_manager.hpp"
#include "logging/logger.hpp"

class MaxDecoderThreadsObservabilityTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Initialize configuration for testing
        auto &config = PocoConfigAdapter::getInstance();

        // Set initial max_decoder_threads
        config.setMaxDecoderThreads(2);

        // Wait for configuration to be applied
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    void TearDown() override
    {
        // Clean up any test-specific configuration
        auto &config = PocoConfigAdapter::getInstance();
        config.setMaxDecoderThreads(2); // Reset to default
    }
};

TEST_F(MaxDecoderThreadsObservabilityTest, MediaDecoderReactsToMaxDecoderThreadsChanges)
{
    auto &media_decoder = MediaDecoder::getInstance();
    auto &config = PocoConfigAdapter::getInstance();

    // Subscribe MediaDecoder to configuration changes first
    config.subscribe(&media_decoder);

    // Refresh configuration to ensure we have the latest values
    media_decoder.refreshConfiguration();

    // Verify initial value
    int initial_threads = config.getMaxDecoderThreads();
    EXPECT_EQ(initial_threads, 2);
    EXPECT_EQ(media_decoder.getMaxDecoderThreads(), 2);

    // Now change max_decoder_threads after subscription
    int new_threads = 4;
    config.setMaxDecoderThreads(new_threads);

    // Wait for configuration change to be processed
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Verify the change was applied
    int current_threads = config.getMaxDecoderThreads();
    EXPECT_EQ(current_threads, new_threads);
    EXPECT_EQ(media_decoder.getMaxDecoderThreads(), new_threads);

    // Unsubscribe to avoid affecting other tests
    config.unsubscribe(&media_decoder);
}

TEST_F(MaxDecoderThreadsObservabilityTest, TranscodingManagerReactsToMaxDecoderThreadsChanges)
{
    auto &transcoding_manager = TranscodingManager::getInstance();
    auto &config = PocoConfigAdapter::getInstance();

    // Subscribe TranscodingManager to configuration changes
    config.subscribe(&transcoding_manager);

    // Verify initial value
    int initial_threads = config.getMaxDecoderThreads();
    EXPECT_EQ(initial_threads, 2);

    // Change max_decoder_threads
    int new_threads = 6;
    config.setMaxDecoderThreads(new_threads);

    // Wait for configuration change to be processed
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Verify the change was applied
    int current_threads = config.getMaxDecoderThreads();
    EXPECT_EQ(current_threads, new_threads);

    // Unsubscribe to avoid affecting other tests
    config.unsubscribe(&transcoding_manager);
}

TEST_F(MaxDecoderThreadsObservabilityTest, BothComponentsReactToMaxDecoderThreadsChanges)
{
    auto &media_decoder = MediaDecoder::getInstance();
    auto &transcoding_manager = TranscodingManager::getInstance();
    auto &config = PocoConfigAdapter::getInstance();

    // Subscribe both components to configuration changes
    config.subscribe(&media_decoder);
    config.subscribe(&transcoding_manager);

    // Refresh MediaDecoder configuration to ensure we have the latest values
    media_decoder.refreshConfiguration();

    // Verify initial value
    int initial_threads = config.getMaxDecoderThreads();
    EXPECT_EQ(initial_threads, 2);
    EXPECT_EQ(media_decoder.getMaxDecoderThreads(), 2);

    // Change max_decoder_threads
    int new_threads = 8;
    config.setMaxDecoderThreads(new_threads);

    // Wait for configuration change to be processed
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Verify the change was applied
    int current_threads = config.getMaxDecoderThreads();
    EXPECT_EQ(current_threads, new_threads);
    EXPECT_EQ(media_decoder.getMaxDecoderThreads(), new_threads);

    // Unsubscribe to avoid affecting other tests
    config.unsubscribe(&media_decoder);
    config.unsubscribe(&transcoding_manager);
}

TEST_F(MaxDecoderThreadsObservabilityTest, ConfigurationChangeTriggersImmediateNotification)
{
    auto &config = PocoConfigAdapter::getInstance();

    // Create a mock observer to verify notifications
    class MockObserver : public ConfigObserver
    {
    public:
        std::atomic<bool> notified{false};
        std::vector<std::string> changed_keys;

        void onConfigUpdate(const ConfigUpdateEvent &event) override
        {
            notified.store(true);
            changed_keys = event.changed_keys;
        }
    };

    MockObserver mock_observer;
    config.subscribe(&mock_observer);

    // Verify initial state
    EXPECT_FALSE(mock_observer.notified.load());
    EXPECT_TRUE(mock_observer.changed_keys.empty());

    // Change max_decoder_threads
    config.setMaxDecoderThreads(10);

    // Wait for configuration change to be processed
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Verify notification was received
    EXPECT_TRUE(mock_observer.notified.load());
    EXPECT_FALSE(mock_observer.changed_keys.empty());

    // Verify the correct key was changed
    auto it = std::find(mock_observer.changed_keys.begin(),
                        mock_observer.changed_keys.end(),
                        "max_decoder_threads");
    EXPECT_NE(it, mock_observer.changed_keys.end());

    // Unsubscribe
    config.unsubscribe(&mock_observer);
}

TEST_F(MaxDecoderThreadsObservabilityTest, MultipleRapidChangesAreHandledCorrectly)
{
    auto &config = PocoConfigAdapter::getInstance();

    // Create a mock observer to count notifications
    class MockObserver : public ConfigObserver
    {
    public:
        std::atomic<int> notification_count{0};

        void onConfigUpdate(const ConfigUpdateEvent &event) override
        {
            notification_count.fetch_add(1);
        }
    };

    MockObserver mock_observer;
    config.subscribe(&mock_observer);

    // Verify initial state
    EXPECT_EQ(mock_observer.notification_count.load(), 0);

    // Make multiple rapid changes
    config.setMaxDecoderThreads(3);
    config.setMaxDecoderThreads(5);
    config.setMaxDecoderThreads(7);

    // Wait for configuration changes to be processed
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Verify all changes were processed
    EXPECT_EQ(mock_observer.notification_count.load(), 3);

    // Verify final value
    int final_threads = config.getMaxDecoderThreads();
    EXPECT_EQ(final_threads, 7);

    // Unsubscribe
    config.unsubscribe(&mock_observer);
}
