#include <gtest/gtest.h>
#include "core/status.hpp"

class StatusTest : public ::testing::Test
{
protected:
    Status status;
};

TEST_F(StatusTest, CheckStatusReturnsTrue)
{
    EXPECT_TRUE(status.checkStatus());
}

TEST_F(StatusTest, CheckStatusIsConsistent)
{
    bool firstResult = status.checkStatus();
    bool secondResult = status.checkStatus();
    EXPECT_EQ(firstResult, secondResult);
    EXPECT_TRUE(firstResult);
}