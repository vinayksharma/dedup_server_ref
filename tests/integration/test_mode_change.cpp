#include <iostream>
#include <thread>
#include <chrono>
#include "../../include/core/poco_config_adapter.hpp"
#include "../../include/core/config_observer.hpp"
#include "../../include/core/dedup_modes.hpp"
#include "../../include/logging/logger.hpp"

class TestConfigObserver : public ConfigObserver
{
public:
    void onConfigUpdate(const ConfigUpdateEvent &event) override
    {
        // Check if dedup_mode was changed
        for (const auto &key : event.changed_keys)
        {
            if (key == "dedup_mode")
            {
                std::cout << "=== CONFIG CHANGE DETECTED ===" << std::endl;
                std::cout << "Event: dedup_mode changed" << std::endl;
                std::cout << "Source: " << event.source << std::endl;
                std::cout << "Update ID: " << event.update_id << std::endl;
                std::cout << "=============================" << std::endl;
                break;
            }
        }
    }
};

int main()
{
    // Initialize logger
    Logger::init("INFO");

    std::cout << "=== Testing Deduplication Mode Change Detection ===" << std::endl;

    // Get current dedup mode
    auto &config_manager = PocoConfigAdapter::getInstance();
    auto current_mode = config_manager.getDedupMode();
    std::string mode_name = DedupModes::getModeName(current_mode);
    std::cout << "Current dedup mode: " << mode_name << std::endl;

    // Subscribe to configuration changes
    TestConfigObserver observer;
    config_manager.subscribe(&observer);
    std::cout << "Subscribed to configuration changes" << std::endl;

    // Start watching config file
    config_manager.startWatching("config.json", 1);
    std::cout << "Started watching config.json" << std::endl;

    // Test 1: Change mode via API
    std::cout << "\n=== Test 1: Changing mode via API ===" << std::endl;
    std::cout << "Changing mode from " << mode_name << " to FAST..." << std::endl;
    config_manager.setDedupMode(DedupMode::FAST);

    // Wait a moment
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Check if mode changed
    current_mode = config_manager.getDedupMode();
    mode_name = DedupModes::getModeName(current_mode);
    std::cout << "Mode after API change: " << mode_name << std::endl;

    // Test 2: Change mode via API again
    std::cout << "\n=== Test 2: Changing mode via API again ===" << std::endl;
    std::cout << "Changing mode from " << mode_name << " to BALANCED..." << std::endl;
    config_manager.setDedupMode(DedupMode::BALANCED);

    // Wait a moment
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Check if mode changed
    current_mode = config_manager.getDedupMode();
    mode_name = DedupModes::getModeName(current_mode);
    std::cout << "Mode after second API change: " << mode_name << std::endl;

    // Test 3: Change mode via API to QUALITY
    std::cout << "\n=== Test 3: Changing mode via API to QUALITY ===" << std::endl;
    std::cout << "Changing mode from " << mode_name << " to QUALITY..." << std::endl;
    config_manager.setDedupMode(DedupMode::QUALITY);

    // Wait a moment
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Check if mode changed
    current_mode = config_manager.getDedupMode();
    mode_name = DedupModes::getModeName(current_mode);
    std::cout << "Mode after third API change: " << mode_name << std::endl;

    // Test 4: Check if file processing would use the new mode
    std::cout << "\n=== Test 4: Checking if components would use new mode ===" << std::endl;

    // Simulate what happens in file processing
    for (int i = 0; i < 3; i++)
    {
        auto mode = config_manager.getDedupMode();
        std::string mode_name = DedupModes::getModeName(mode);
        std::cout << "File processing would use mode: " << mode_name << std::endl;

        // Change mode
        if (mode == DedupMode::FAST)
            config_manager.setDedupMode(DedupMode::BALANCED);
        else if (mode == DedupMode::BALANCED)
            config_manager.setDedupMode(DedupMode::QUALITY);
        else
            config_manager.setDedupMode(DedupMode::FAST);

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    // Stop watching
    config_manager.stopWatching();
    std::cout << "\nStopped watching config.json" << std::endl;

    std::cout << "\n=== Test Complete ===" << std::endl;
    return 0;
}
