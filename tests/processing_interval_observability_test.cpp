#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <atomic>
#include "core/simple_scheduler.hpp"
#include "core/duplicate_linker.hpp"
#include "poco_config_adapter.hpp"
#include "database/database_manager.hpp"
#include "logging/logger.hpp"

class ProcessingIntervalObservabilityTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Initialize configuration for testing
        auto &config = PocoConfigAdapter::getInstance();

        // Set initial processing interval
        config.setProcessingIntervalSeconds(900);

        // Wait for configuration to be applied
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    void TearDown() override
    {
        // Restore default configuration
        auto &config = PocoConfigAdapter::getInstance();
        config.setProcessingIntervalSeconds(900);
    }
};

TEST_F(ProcessingIntervalObservabilityTest, SimpleSchedulerReactsToProcessingIntervalChanges)
{
    auto &scheduler = SimpleScheduler::getInstance();
    auto &config = PocoConfigAdapter::getInstance();

    // Subscribe scheduler to configuration changes
    config.subscribe(&scheduler);

    // Verify initial interval
    int initial_interval = config.getProcessingIntervalSeconds();
    EXPECT_EQ(initial_interval, 900);

    // Change processing interval
    int new_interval = 300; // 5 minutes
    config.setProcessingIntervalSeconds(new_interval);

    // Wait for configuration change to be processed
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Verify the change was applied
    int current_interval = config.getProcessingIntervalSeconds();
    EXPECT_EQ(current_interval, new_interval);

    // Unsubscribe to avoid affecting other tests
    config.unsubscribe(&scheduler);
}

TEST_F(ProcessingIntervalObservabilityTest, DuplicateLinkerReactsToProcessingIntervalChanges)
{
    auto &duplicate_linker = DuplicateLinker::getInstance();
    auto &config = PocoConfigAdapter::getInstance();

    // Subscribe duplicate linker to configuration changes
    config.subscribe(&duplicate_linker);

    // Verify initial interval
    int initial_interval = config.getProcessingIntervalSeconds();
    EXPECT_EQ(initial_interval, 900);

    // Change processing interval
    int new_interval = 600; // 10 minutes
    config.setProcessingIntervalSeconds(new_interval);

    // Wait for configuration change to be processed
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Verify the change was applied
    int current_interval = config.getProcessingIntervalSeconds();
    EXPECT_EQ(current_interval, new_interval);

    // Unsubscribe to avoid affecting other tests
    config.unsubscribe(&duplicate_linker);
}

TEST_F(ProcessingIntervalObservabilityTest, BothComponentsReactToSameConfigurationChange)
{
    auto &scheduler = SimpleScheduler::getInstance();
    auto &duplicate_linker = DuplicateLinker::getInstance();
    auto &config = PocoConfigAdapter::getInstance();

    // Subscribe both components to configuration changes
    config.subscribe(&scheduler);
    config.subscribe(&duplicate_linker);

    // Verify initial interval
    int initial_interval = config.getProcessingIntervalSeconds();
    EXPECT_EQ(initial_interval, 900);

    // Change processing interval
    int new_interval = 1800; // 30 minutes
    config.setProcessingIntervalSeconds(new_interval);

    // Wait for configuration change to be processed
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Verify the change was applied
    int current_interval = config.getProcessingIntervalSeconds();
    EXPECT_EQ(current_interval, new_interval);

    // Unsubscribe to avoid affecting other tests
    config.unsubscribe(&scheduler);
    config.unsubscribe(&duplicate_linker);
}

TEST_F(ProcessingIntervalObservabilityTest, ConfigurationChangeTriggersImmediateNotification)
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

    // Change processing interval
    config.setProcessingIntervalSeconds(450);

    // Wait for configuration change to be processed
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Verify notification was received
    EXPECT_TRUE(mock_observer.notified.load());
    EXPECT_FALSE(mock_observer.changed_keys.empty());

    // Verify the correct key was changed
    auto it = std::find(mock_observer.changed_keys.begin(),
                        mock_observer.changed_keys.end(),
                        "processing_interval_seconds");
    EXPECT_NE(it, mock_observer.changed_keys.end());

    // Unsubscribe
    config.unsubscribe(&mock_observer);
}

TEST_F(ProcessingIntervalObservabilityTest, MultipleRapidChangesAreHandledCorrectly)
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
    config.setProcessingIntervalSeconds(300);
    config.setProcessingIntervalSeconds(600);
    config.setProcessingIntervalSeconds(900);

    // Wait for configuration changes to be processed
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Verify all changes were processed
    EXPECT_EQ(mock_observer.notification_count.load(), 3);

    // Verify final value
    int final_interval = config.getProcessingIntervalSeconds();
    EXPECT_EQ(final_interval, 900);

    // Unsubscribe
    config.unsubscribe(&mock_observer);
}
