#include "core/scan_config_observer.hpp"
#include "poco_config_adapter.hpp"
#include "config_observer.hpp"
#include <iostream>
#include <memory>

int main()
{
    std::cout << "Testing ScanConfigObserver..." << std::endl;

    try
    {
        // Get configuration manager instance
        auto &config_manager = PocoConfigAdapter::getInstance();

        // Create and register observer
        auto observer = std::make_unique<ScanConfigObserver>();
        config_manager.subscribe(observer.get());

        std::cout << "Observer registered successfully" << std::endl;

        // Test scan interval change
        std::cout << "\nTesting scan interval change..." << std::endl;
        int original_interval = config_manager.getScanIntervalSeconds();
        std::cout << "Original scan interval: " << original_interval << " seconds" << std::endl;

        // Change the scan interval
        config_manager.setScanIntervalSeconds(600); // 10 minutes

        // Test scan thread count change
        std::cout << "\nTesting scan thread count change..." << std::endl;
        int original_threads = config_manager.getMaxScanThreads();
        std::cout << "Original scan threads: " << original_threads << std::endl;

        // Change the scan thread count
        config_manager.setMaxScanThreads(8);

        // Verify configuration was persisted
        std::cout << "\nVerifying configuration persistence..." << std::endl;
        int current_interval = config_manager.getScanIntervalSeconds();
        int current_threads = config_manager.getMaxScanThreads();

        if (current_interval == 600)
        {
            std::cout << "✅ Scan interval persisted correctly: " << current_interval << " seconds" << std::endl;
        }
        else
        {
            std::cout << "❌ Scan interval not persisted correctly. Expected: 600, Got: " << current_interval << std::endl;
        }

        if (current_threads == 8)
        {
            std::cout << "✅ Scan thread count persisted correctly: " << current_threads << std::endl;
        }
        else
        {
            std::cout << "❌ Scan thread count not persisted correctly. Expected: 8, Got: " << current_threads << std::endl;
        }

        // Unregister observer
        config_manager.unsubscribe(observer.get());
        std::cout << "\nObserver unregistered successfully" << std::endl;

        std::cout << "\n🎉 ScanConfigObserver test completed successfully!" << std::endl;
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "❌ Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
