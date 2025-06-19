#pragma once

#include <string>
#include <stdexcept>
#include <jwt-cpp/jwt.h>

class Auth
{
public:
    // Error classes
    class InvalidCredentialsError : public std::runtime_error
    {
    public:
        InvalidCredentialsError() : std::runtime_error("Invalid credentials") {}
    };

    class InvalidTokenError : public std::runtime_error
    {
    public:
        InvalidTokenError() : std::runtime_error("Invalid token") {}
    };

    Auth(const std::string &secret_key) : secret_key_(secret_key) {}

    // Authenticate a user and return a JWT token
    std::string authenticate(const std::string &username, const std::string &password);

    // Generate a JWT token for a user
    std::string generateToken(const std::string &username);

    // Verify a JWT token
    bool verifyToken(const std::string &token);

    // Get username from token
    std::string getUsernameFromToken(const std::string &token);

private:
    std::string secret_key_;
};