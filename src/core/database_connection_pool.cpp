#include "core/database_connection_pool.hpp"
#include "logging/logger.hpp"
#include <algorithm>
#include <chrono>

// Include DatabaseManager header only when not in test mode
#ifndef TEST_MODE
#include "database/database_manager.hpp"
#endif

// Static instance pointer
static std::unique_ptr<DatabaseConnectionPool> instance_ptr;

DatabaseConnectionPool::DatabaseConnectionPool()
{
    Logger::info("DatabaseConnectionPool constructor called");
}

DatabaseConnectionPool::~DatabaseConnectionPool()
{
    shutdown();
    Logger::info("DatabaseConnectionPool destructor called");
}

DatabaseConnectionPool &DatabaseConnectionPool::getInstance()
{
    if (!instance_ptr)
    {
        instance_ptr = std::make_unique<DatabaseConnectionPool>();
    }
    return *instance_ptr;
}

bool DatabaseConnectionPool::initialize(size_t num_connections)
{
    if (initialized_.load())
    {
        Logger::warn("DatabaseConnectionPool: Already initialized with " +
                     std::to_string(current_connection_count_.load()) + " connections");
        return true;
    }

    if (!validateConnectionCount(num_connections))
    {
        Logger::error("DatabaseConnectionPool: Invalid connection count: " + std::to_string(num_connections));
        return false;
    }

    try
    {
        std::lock_guard<std::mutex> lock(resize_mutex_);

        createConnections(num_connections);
        current_connection_count_.store(num_connections);
        initialized_.store(true);

        Logger::info("DatabaseConnectionPool: Successfully initialized with " +
                     std::to_string(num_connections) + " connections");
        return true;
    }
    catch (const std::exception &e)
    {
        Logger::error("DatabaseConnectionPool: Failed to initialize: " + std::string(e.what()));
        resetPool();
        return false;
    }
}

bool DatabaseConnectionPool::resizeConnectionPool(size_t new_num_connections)
{
    if (!initialized_.load())
    {
        Logger::warn("DatabaseConnectionPool: Not initialized. Initializing with " +
                     std::to_string(new_num_connections) + " connections");
        return initialize(new_num_connections);
    }

    if (!validateConnectionCount(new_num_connections))
    {
        Logger::error("DatabaseConnectionPool: Invalid connection count: " + std::to_string(new_num_connections));
        return false;
    }

    try
    {
        std::lock_guard<std::mutex> lock(resize_mutex_);

        size_t current_count = current_connection_count_.load();
        if (current_count == new_num_connections)
        {
            Logger::info("DatabaseConnectionPool: Connection count unchanged: " + std::to_string(new_num_connections));
            return true;
        }

        Logger::info("DatabaseConnectionPool: Resizing connection pool from " +
                     std::to_string(current_count) + " to " + std::to_string(new_num_connections) + " connections");

        if (new_num_connections > current_count)
        {
            // Add new connections
            size_t connections_to_add = new_num_connections - current_count;
            createConnections(connections_to_add);
        }
        else
        {
            // Remove excess connections
            size_t connections_to_remove = current_count - new_num_connections;
            destroyConnections(connections_to_remove);
        }

        current_connection_count_.store(new_num_connections);

        Logger::info("DatabaseConnectionPool: Successfully resized to " +
                     std::to_string(new_num_connections) + " connections");
        return true;
    }
    catch (const std::exception &e)
    {
        Logger::error("DatabaseConnectionPool: Failed to resize connection pool: " + std::string(e.what()));
        return false;
    }
}

void DatabaseConnectionPool::shutdown()
{
    std::lock_guard<std::mutex> lock(resize_mutex_);

    resetPool();
    Logger::info("DatabaseConnectionPool: Shutdown complete");
}

std::shared_ptr<DatabaseManager> DatabaseConnectionPool::acquireConnection()
{
    if (!initialized_.load())
    {
        Logger::error("DatabaseConnectionPool: Not initialized");
        return nullptr;
    }

    std::unique_lock<std::mutex> lock(pool_mutex_);

    // Wait for an available connection
    connection_available_.wait(lock, [this]()
                               { return !available_connections_.empty(); });

    if (available_connections_.empty())
    {
        Logger::error("DatabaseConnectionPool: No available connections despite wait");
        return nullptr;
    }

    auto connection = available_connections_.front();
    available_connections_.pop();

    Logger::debug("DatabaseConnectionPool: Acquired connection. Available: " +
                  std::to_string(available_connections_.size()) +
                  ", Active: " + std::to_string(getActiveConnectionCount()));

    return connection;
}

void DatabaseConnectionPool::releaseConnection(std::shared_ptr<DatabaseManager> connection)
{
    if (!connection)
    {
        Logger::warn("DatabaseConnectionPool: Attempted to release null connection");
        return;
    }

    std::lock_guard<std::mutex> lock(pool_mutex_);

    // Return connection to available pool
    available_connections_.push(connection);

    Logger::debug("DatabaseConnectionPool: Released connection. Available: " +
                  std::to_string(available_connections_.size()) +
                  ", Active: " + std::to_string(getActiveConnectionCount()));

    // Notify waiting threads
    connection_available_.notify_one();
}

size_t DatabaseConnectionPool::getAvailableConnectionCount() const
{
    std::lock_guard<std::mutex> lock(pool_mutex_);
    return available_connections_.size();
}

size_t DatabaseConnectionPool::getActiveConnectionCount() const
{
    return current_connection_count_.load() - getAvailableConnectionCount();
}

bool DatabaseConnectionPool::validateConnectionCount(size_t num_connections)
{
    if (num_connections < MIN_CONNECTIONS || num_connections > MAX_CONNECTIONS)
    {
        Logger::warn("Database connection count " + std::to_string(num_connections) +
                     " is outside valid range [" + std::to_string(MIN_CONNECTIONS) +
                     ", " + std::to_string(MAX_CONNECTIONS) + "]");
        return false;
    }

    // Check if the number is reasonable for the system
    size_t hardware_concurrency = std::thread::hardware_concurrency();
    if (num_connections > hardware_concurrency * 2)
    {
        Logger::warn("Database connection count " + std::to_string(num_connections) +
                     " is significantly higher than hardware concurrency (" +
                     std::to_string(hardware_concurrency) + "). This may impact performance.");
    }

    return true;
}

void DatabaseConnectionPool::createConnections(size_t num_connections)
{
    for (size_t i = 0; i < num_connections; ++i)
    {
        try
        {
            if (test_mode_.load())
            {
                // In test mode, create a null shared_ptr to avoid external dependencies
                connections_.push_back(std::shared_ptr<DatabaseManager>(nullptr));
                available_connections_.push(connections_.back());

                Logger::debug("DatabaseConnectionPool: Created test connection " + std::to_string(i + 1));
            }
#ifdef TEST_MODE
            else
            {
                // This branch should never be reached in test mode, but just in case
                connections_.push_back(std::shared_ptr<DatabaseManager>(nullptr));
                available_connections_.push(connections_.back());

                Logger::debug("DatabaseConnectionPool: Created test connection (fallback) " + std::to_string(i + 1));
            }
#else
            else
            {
                // In normal mode, store a reference to the singleton DatabaseManager instance
                // Since DatabaseManager is a singleton, we don't need to copy it
                auto db_manager_ref = &DatabaseManager::getInstance();
                connections_.push_back(std::shared_ptr<DatabaseManager>(db_manager_ref, [](DatabaseManager *)
                                                                        {
                                                                            // Custom deleter that does nothing since we don't own the singleton
                                                                        }));
                available_connections_.push(connections_.back());

                Logger::debug("DatabaseConnectionPool: Created connection " + std::to_string(i + 1));
            }
#endif
        }
        catch (const std::exception &e)
        {
            Logger::error("DatabaseConnectionPool: Failed to create connection " + std::to_string(i + 1) +
                          ": " + std::string(e.what()));
            throw;
        }
    }
}

void DatabaseConnectionPool::destroyConnections(size_t num_connections)
{
    size_t connections_to_remove = std::min(num_connections, connections_.size());

    // Remove connections from the end of the vector
    for (size_t i = 0; i < connections_to_remove; ++i)
    {
        if (!connections_.empty())
        {
            connections_.pop_back();
            Logger::debug("DatabaseConnectionPool: Destroyed connection");
        }
    }

    // Clear the available connections queue and repopulate with remaining connections
    std::queue<std::shared_ptr<DatabaseManager>> temp_queue;
    available_connections_.swap(temp_queue);

    for (const auto &connection : connections_)
    {
        available_connections_.push(connection);
    }
}

void DatabaseConnectionPool::resetPool()
{
    connections_.clear();

    std::queue<std::shared_ptr<DatabaseManager>> temp_queue;
    available_connections_.swap(temp_queue);

    current_connection_count_.store(0);
    initialized_.store(false);
}
