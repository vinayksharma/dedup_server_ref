#pragma once

#include "config_observer.hpp"
#include <string>

/**
 * @brief Observer for threading-related configuration changes
 *
 * This observer reacts to changes in threading configuration settings,
 * such as max processing threads, max scan threads, database threads, etc.
 */
class ThreadingConfigObserver : public ConfigObserver
{
public:
    /**
     * @brief Constructor
     */
    ThreadingConfigObserver() = default;

    /**
     * @brief Destructor
     */
    ~ThreadingConfigObserver() = default;

    /**
     * @brief Handle configuration update events
     * @param event Configuration update event
     */
    void onConfigUpdate(const ConfigUpdateEvent &event) override;

private:
    /**
     * @brief Check if the event contains processing threads changes
     * @param event Configuration update event
     * @return true if processing threads changed
     */
    bool hasProcessingThreadsChange(const ConfigUpdateEvent &event) const;

    /**
     * @brief Check if the event contains scan threads changes
     * @param event Configuration update event
     * @return true if scan threads changed
     */
    bool hasScanThreadsChange(const ConfigUpdateEvent &event) const;

    /**
     * @brief Check if the event contains database threads changes
     * @param event Configuration update event
     * @return true if database threads changed
     */
    bool hasDatabaseThreadsChange(const ConfigUpdateEvent &event) const;

    /**
     * @brief Handle processing threads configuration change
     * @param new_thread_count New processing threads value
     */
    void handleProcessingThreadsChange(int new_thread_count);

    /**
     * @brief Handle scan threads configuration change
     * @param new_thread_count New scan threads value
     */
    void handleScanThreadsChange(int new_thread_count);

    /**
     * @brief Handle database threads configuration change
     * @param new_thread_count New database threads value
     */
    void handleDatabaseThreadsChange(int new_thread_count);
};
