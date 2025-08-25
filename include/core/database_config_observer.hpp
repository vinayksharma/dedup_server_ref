#pragma once

#include "config_observer.hpp"
#include "logging/logger.hpp"

/**
 * @brief Observer for database-related configuration changes
 *
 * This observer reacts to changes in database configurations
 * such as retry settings, timeouts, and other database settings.
 */
class DatabaseConfigObserver : public ConfigObserver
{
public:
    DatabaseConfigObserver() = default;
    ~DatabaseConfigObserver() override = default;

    /**
     * @brief Handle configuration changes
     * @param event Configuration update event
     */
    void onConfigUpdate(const ConfigUpdateEvent &event) override;

private:
    /**
     * @brief Check if the event contains database retry changes
     * @param event Configuration update event
     * @return true if database retry settings changed
     */
    bool hasDatabaseRetryChange(const ConfigUpdateEvent &event) const;

    /**
     * @brief Check if the event contains database timeout changes
     * @param event Configuration update event
     * @return true if database timeout settings changed
     */
    bool hasDatabaseTimeoutChange(const ConfigUpdateEvent &event) const;

    /**
     * @brief Handle database retry configuration change
     */
    void handleDatabaseRetryChange();

    /**
     * @brief Handle database timeout configuration change
     */
    void handleDatabaseTimeoutChange();
};
