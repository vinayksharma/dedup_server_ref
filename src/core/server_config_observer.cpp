#include "core/server_config_observer.hpp"
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

    // TODO: Implement actual server port change logic
    // This would involve:
    // 1. Stopping the current HTTP server
    // 2. Reconfiguring the server with the new port
    // 3. Restarting the server on the new port
    // 4. Logging the change for audit purposes

    Logger::warn("ServerConfigObserver: Server port change detected but not yet implemented. "
                 "Server restart required to apply new port: " +
                 std::to_string(new_port));
}

void ServerConfigObserver::handleServerHostChange(const std::string &new_host)
{
    Logger::info("ServerConfigObserver: Server host configuration changed to: " + new_host);

    // TODO: Implement actual server host change logic
    // This would involve:
    // 1. Updating the server binding configuration
    // 2. Restarting the server with the new host binding
    // 3. Logging the change for audit purposes

    Logger::warn("ServerConfigObserver: Server host change detected but not yet implemented. "
                 "Server restart required to apply new host: " +
                 new_host);
}
