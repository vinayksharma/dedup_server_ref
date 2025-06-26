#include <gtest/gtest.h>
#include "auth/auth.hpp"

class AuthTest : public ::testing::Test
{
protected:
    Auth auth{"test-secret-key"};
};

TEST_F(AuthTest, TokenGeneration)
{
    std::string token = auth.generateToken("testuser");
    EXPECT_FALSE(token.empty());
}

TEST_F(AuthTest, TokenVerification)
{
    std::string token = auth.generateToken("testuser");
    EXPECT_TRUE(auth.verifyToken(token));
}

TEST_F(AuthTest, InvalidToken)
{
    EXPECT_FALSE(auth.verifyToken("invalid-token"));
}

TEST_F(AuthTest, GetUsernameFromToken)
{
    std::string username = "testuser";
    std::string token = auth.generateToken(username);
    EXPECT_EQ(auth.getUsernameFromToken(token), username);
}

TEST_F(AuthTest, GetUsernameFromInvalidToken)
{
    EXPECT_THROW(auth.getUsernameFromToken("invalid-token"), Auth::InvalidTokenError);
}