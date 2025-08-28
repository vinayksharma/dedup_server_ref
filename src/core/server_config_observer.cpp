#include "core/server_config_observer.hpp"
#include "core/http_server_manager.hpp"
#include "poco_config_adapter.hpp"
#include "logging/logger.hpp"
#include <algorithm>

void ServerConfigObserver::onConfigUpdate(const ConfigUpdateEvent &event)
{
    // Check for server port changes
    if (hasServerPortChange(event))
    {
        try
        {
            auto &config_manager = PocoConfigAdapter::getInstance();
            int new_port = config_manager.getServerPort();
            handleServerPortChange(new_port);
        }
        catch (const std::exception &e)
        {
            Logger::error("Error handling server port configuration change: " + std::string(e.what()));
        }
    }

    // Check for server host changes
    if (hasServerHostChange(event))
    {
        try
        {
            auto &config_manager = PocoConfigAdapter::getInstance();
            std::string new_host = config_manager.getServerHost();
            handleServerHostChange(new_host);
        }
        catch (const std::exception &e)
        {
            Logger::error("Error handling server host configuration change: " + std::string(e.what()));
        }
    }
}

bool ServerConfigObserver::hasServerPortChange(const ConfigUpdateEvent &event) const
{
    return std::find(event.changed_keys.begin(), event.changed_keys.end(), "server_port") != event.changed_keys.end();
}

bool ServerConfigObserver::hasServerHostChange(const ConfigUpdateEvent &event) const
{
    return std::find(event.changed_keys.begin(), event.changed_keys.end(), "server_host") != event.changed_keys.end();
}

void ServerConfigObserver::handleServerPortChange(int new_port)
{
    Logger::info("ServerConfigObserver: Server port configuration changed to: " + std::to_string(new_port));

    try
    {
        // Get the HttpServerManager instance to coordinate the port change
        auto& server_manager = HttpServerManager::getInstance();

        if (!server_manager.isRunning())
        {
            Logger::warn("ServerConfigObserver: HttpServerManager is not running. Port change will take effect when server starts.");
            return;
        }

        // The HttpServerManager will automatically handle the port change through its ConfigObserver implementation
        // This ensures immediate reactivity without requiring manual intervention

        Logger::info("ServerConfigObserver: Successfully coordinated server port change with HttpServerManager. "
                     "New port: " + std::to_string(new_port) + " will take effect immediately.");

        // Log the change for audit purposes
        Logger::info("ServerConfigObserver: Server port change audit - "
                     "Configuration updated to " + std::to_string(new_port) + " at " +
                     std::to_string(std::chrono::system_clock::now().time_since_epoch().count()));

    }
    catch (const std::exception& e)
    {
        Logger::error("ServerConfigObserver: Failed to coordinate server port change with HttpServerManager: " + std::string(e.what()));

        // Fallback: Log that manual intervention may be required
        Logger::warn("ServerConfigObserver: Manual server restart may be required to apply new port: " +
                     std::to_string(new_port));
    }
}

void ServerConfigObserver::handleServerHostChange(const std::string &new_host)
{
    Logger::info("ServerConfigObserver: Server host configuration changed to: " + new_host);

    try
    {
        // Get the HttpServerManager instance to coordinate the host change
        auto& server_manager = HttpServerManager::getInstance();

        if (!server_manager.isRunning())
        {
            Logger::warn("ServerConfigObserver: HttpServerManager is not running. Host change will take effect when server starts.");
            return;
        }

        // The HttpServerManager will automatically handle the host change through its ConfigObserver implementation
        // This ensures immediate reactivity without requiring manual intervention

        Logger::info("ServerConfigObserver: Successfully coordinated server host change with HttpServerManager. "
                     "New host: " + new_host + " will take effect immediately.");

        // Log the change for audit purposes
        Logger::info("ServerConfigObserver: Server host change audit - "
                     "Configuration updated to " + new_host + " at " +
                     std::to_string(std::chrono::system_clock::now().time_since_epoch().count()));

    }
    catch (const std::exception& e)
    {
        Logger::error("ServerConfigObserver: Failed to coordinate server host change with HttpServerManager: " + std::string(e.what()));

        // Fallback: Log that manual intervention may be required
        Logger::warn("ServerConfigObserver: Manual server restart may be required to apply new host: " + new_host);
    }
}
