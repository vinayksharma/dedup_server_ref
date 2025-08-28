#pragma once

#include <memory>
#include <atomic>
#include <mutex>
#include <vector>
#include <queue>
#include <condition_variable>
#include <functional>
#include "logging/logger.hpp"

// Forward declaration to avoid including the full DatabaseManager header
class DatabaseManager;

/**
 * @brief Manages database connections with dynamic resizing capability
 *
 * This class provides a centralized way to manage database connections
 * and allows dynamic resizing based on configuration changes.
 */
class DatabaseConnectionPool
{
public:
    // Singleton pattern
    static DatabaseConnectionPool &getInstance();

    // Constructor (made public for std::make_unique)
    DatabaseConnectionPool();

    // Connection pool management
    bool initialize(size_t num_connections);
    bool resizeConnectionPool(size_t new_num_connections);
    void shutdown();

    // Connection management
    std::shared_ptr<DatabaseManager> acquireConnection();
    void releaseConnection(std::shared_ptr<DatabaseManager> connection);

    // Getters
    size_t getCurrentConnectionCount() const { return current_connection_count_.load(); }
    size_t getAvailableConnectionCount() const;
    size_t getActiveConnectionCount() const;
    bool isInitialized() const { return initialized_.load(); }

    // Configuration validation
    static bool validateConnectionCount(size_t num_connections);

    // Test mode support
    void setTestMode(bool test_mode) { test_mode_ = test_mode; }
    bool isTestMode() const { return test_mode_; }

    // Destructor
    ~DatabaseConnectionPool();

private:
    // Disable copy and assignment
    DatabaseConnectionPool(const DatabaseConnectionPool &) = delete;
    DatabaseConnectionPool &operator=(const DatabaseConnectionPool &) = delete;

    // Member variables
    std::atomic<bool> initialized_{false};
    std::atomic<size_t> current_connection_count_{0};
    std::vector<std::shared_ptr<DatabaseManager>> connections_;
    std::queue<std::shared_ptr<DatabaseManager>> available_connections_;
    std::atomic<bool> test_mode_{false};

    // Thread safety
    mutable std::mutex pool_mutex_;
    mutable std::mutex resize_mutex_;
    std::condition_variable connection_available_;

    // Constants
    static constexpr size_t MIN_CONNECTIONS = 1;
    static constexpr size_t MAX_CONNECTIONS = 32;
    static constexpr size_t DEFAULT_CONNECTIONS = 2;

    // Helper methods
    void createConnections(size_t num_connections);
    void destroyConnections(size_t num_connections);
    void resetPool();
};
