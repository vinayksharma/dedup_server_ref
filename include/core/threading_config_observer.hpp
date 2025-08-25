#pragma once

#include "config_observer.hpp"
#include "logging/logger.hpp"

/**
 * @brief Observer for threading-related configuration changes
 *
 * This observer reacts to changes in threading configurations
 * such as max processing threads, max scan threads, database threads, etc.
 */
class ThreadingConfigObserver : public ConfigObserver
{
public:
    ThreadingConfigObserver() = default;
    ~ThreadingConfigObserver() override = default;

    /**
     * @brief Handle configuration changes
     * @param event Configuration update event
     */
    void onConfigUpdate(const ConfigUpdateEvent &event) override;

private:
    /**
     * @brief Check if the event contains max processing threads changes
     * @param event Configuration update event
     * @return true if max processing threads changed
     */
    bool hasMaxProcessingThreadsChange(const ConfigUpdateEvent &event) const;

    /**
     * @brief Check if the event contains max scan threads changes
     * @param event Configuration update event
     * @return true if max scan threads changed
     */
    bool hasMaxScanThreadsChange(const ConfigUpdateEvent &event) const;

    /**
     * @brief Check if the event contains database threads changes
     * @param event Configuration update event
     * @return true if database threads changed
     */
    bool hasDatabaseThreadsChange(const ConfigUpdateEvent &event) const;

    /**
     * @brief Check if the event contains HTTP server threads changes
     * @param event Configuration update event
     * @return true if HTTP server threads changed
     */
    bool hasHttpServerThreadsChange(const ConfigUpdateEvent &event) const;

    /**
     * @brief Handle max processing threads configuration change
     * @param new_thread_count New max processing threads value
     */
    void handleMaxProcessingThreadsChange(int new_thread_count);

    /**
     * @brief Handle max scan threads configuration change
     * @param new_thread_count New max scan threads value
     */
    void handleMaxScanThreadsChange(int new_thread_count);

    /**
     * @brief Handle database threads configuration change
     * @param new_thread_count New database threads value
     */
    void handleDatabaseThreadsChange(int new_thread_count);

    /**
     * @brief Handle HTTP server threads configuration change
     * @param new_threads New HTTP server threads value
     */
    void handleHttpServerThreadsChange(const std::string &new_threads);
};
