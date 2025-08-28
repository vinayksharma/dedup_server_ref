#include <gtest/gtest.h>
#include "../include/core/database_connection_pool.hpp"
#include <thread>
#include <chrono>

class DatabaseConnectionPoolTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Reset the singleton for each test
        auto &pool = DatabaseConnectionPool::getInstance();
        pool.shutdown();
        pool.setTestMode(true); // Enable test mode to avoid external dependencies
    }

    void TearDown() override
    {
        // Clean up after each test
        auto &pool = DatabaseConnectionPool::getInstance();
        pool.shutdown();
    }
};

TEST_F(DatabaseConnectionPoolTest, Initialization)
{
    auto &pool = DatabaseConnectionPool::getInstance();

    EXPECT_FALSE(pool.isInitialized());
    EXPECT_EQ(pool.getCurrentConnectionCount(), 0);

    bool init_result = pool.initialize(2);
    EXPECT_TRUE(init_result);
    EXPECT_TRUE(pool.isInitialized());
    EXPECT_EQ(pool.getCurrentConnectionCount(), 2);
}

TEST_F(DatabaseConnectionPoolTest, ResizeConnectionPool)
{
    auto &pool = DatabaseConnectionPool::getInstance();

    // Initialize with 2 connections
    EXPECT_TRUE(pool.initialize(2));
    EXPECT_EQ(pool.getCurrentConnectionCount(), 2);

    // Resize to 4 connections
    EXPECT_TRUE(pool.resizeConnectionPool(4));
    EXPECT_EQ(pool.getCurrentConnectionCount(), 4);

    // Resize to 1 connection
    EXPECT_TRUE(pool.resizeConnectionPool(1));
    EXPECT_EQ(pool.getCurrentConnectionCount(), 1);
}

TEST_F(DatabaseConnectionPoolTest, Validation)
{
    auto &pool = DatabaseConnectionPool::getInstance();

    // Test invalid connection counts
    EXPECT_FALSE(pool.initialize(0));  // Below minimum
    EXPECT_FALSE(pool.initialize(33)); // Above maximum

    // Test valid connection count
    EXPECT_TRUE(pool.initialize(16));
}

TEST_F(DatabaseConnectionPoolTest, ConnectionManagement)
{
    auto &pool = DatabaseConnectionPool::getInstance();

    EXPECT_TRUE(pool.initialize(2));

    // Test connection counts
    EXPECT_EQ(pool.getCurrentConnectionCount(), 2);
    EXPECT_EQ(pool.getAvailableConnectionCount(), 2);
    EXPECT_EQ(pool.getActiveConnectionCount(), 0);
}

TEST_F(DatabaseConnectionPoolTest, Shutdown)
{
    auto &pool = DatabaseConnectionPool::getInstance();

    EXPECT_TRUE(pool.initialize(3));
    EXPECT_TRUE(pool.isInitialized());
    EXPECT_EQ(pool.getCurrentConnectionCount(), 3);

    pool.shutdown();

    EXPECT_FALSE(pool.isInitialized());
    EXPECT_EQ(pool.getCurrentConnectionCount(), 0);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
