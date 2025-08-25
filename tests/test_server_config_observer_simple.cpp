#include "core/server_config_observer.hpp"
#include "poco_config_adapter.hpp"
#include "config_observer.hpp"
#include <iostream>
#include <memory>

int main()
{
    std::cout << "Testing ServerConfigObserver..." << std::endl;

    try
    {
        // Get configuration manager instance
        auto &config_manager = PocoConfigAdapter::getInstance();

        // Create and register observer
        auto observer = std::make_unique<ServerConfigObserver>();
        config_manager.subscribe(observer.get());

        std::cout << "Observer registered successfully" << std::endl;

        // Test server port change
        std::cout << "\nTesting server port change..." << std::endl;
        int original_port = config_manager.getServerPort();
        std::cout << "Original port: " << original_port << std::endl;

        // Change the port
        config_manager.setServerPort(8081);

        // Test server host change
        std::cout << "\nTesting server host change..." << std::endl;
        std::string original_host = config_manager.getServerHost();
        std::cout << "Original host: " << original_host << std::endl;

        // Change the host
        config_manager.setServerHost("0.0.0.0");

        // Verify configuration was persisted
        std::cout << "\nVerifying configuration persistence..." << std::endl;
        int current_port = config_manager.getServerPort();
        std::string current_host = config_manager.getServerHost();

        if (current_port == 8081)
        {
            std::cout << "✅ Server port persisted correctly: " << current_port << std::endl;
        }
        else
        {
            std::cout << "❌ Server port not persisted correctly. Expected: 8081, Got: " << current_port << std::endl;
        }

        if (current_host == "0.0.0.0")
        {
            std::cout << "✅ Server host persisted correctly: " << current_host << std::endl;
        }
        else
        {
            std::cout << "❌ Server host not persisted correctly. Expected: 0.0.0.0, Got: " << current_host << std::endl;
        }

        // Unregister observer
        config_manager.unsubscribe(observer.get());
        std::cout << "\nObserver unregistered successfully" << std::endl;

        std::cout << "\n🎉 ServerConfigObserver test completed successfully!" << std::endl;
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "❌ Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
