#pragma once

#include "config_observer.hpp"
#include "logging/logger.hpp"

/**
 * @brief Observer for server configuration changes
 *
 * This observer reacts to changes in server-related configurations
 * such as port, host, and other server settings.
 */
class ServerConfigObserver : public ConfigObserver
{
public:
    ServerConfigObserver() = default;
    ~ServerConfigObserver() override = default;

    /**
     * @brief Handle configuration changes
     * @param event Configuration update event
     */
    void onConfigUpdate(const ConfigUpdateEvent &event) override;

private:
    /**
     * @brief Check if the event contains server port changes
     * @param event Configuration update event
     * @return true if server port changed
     */
    bool hasServerPortChange(const ConfigUpdateEvent &event) const;

    /**
     * @brief Check if the event contains server host changes
     * @param event Configuration update event
     * @return true if server host changed
     */
    bool hasServerHostChange(const ConfigUpdateEvent &event) const;

    /**
     * @brief Handle server port configuration change
     * @param new_port New server port value
     */
    void handleServerPortChange(int new_port);

    /**
     * @brief Handle server host configuration change
     * @param new_host New server host value
     */
    void handleServerHostChange(const std::string &new_host);
};
