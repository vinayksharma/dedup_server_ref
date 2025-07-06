#include "auth/auth.hpp"
#include <chrono>
#include <stdexcept>

std::string Auth::authenticate(const std::string &username, const std::string &password)
{
    // TODO: PRODUCTION SECURITY - Implement proper user authentication
    // Current behavior: Hardcoded credentials (username: "admin", password: "password")
    // For production:
    // 1. Use database for user storage
    // 2. Hash passwords with bcrypt/argon2
    // 3. Implement rate limiting
    // 4. Add audit logging
    if (username == "admin" && password == "password")
    {
        return generateToken(username);
    }
    throw InvalidCredentialsError();
}

std::string Auth::generateToken(const std::string &username)
{
    // TODO: PRODUCTION SECURITY - Add token expiration
    // Current behavior: Tokens never expire (no exp claim)
    // For production: Add .set_expires_at(std::chrono::system_clock::now() + std::chrono::hours(24))
    auto token = jwt::create()
                     .set_issuer("dedup_server")
                     .set_type("JWS")
                     .set_payload_claim("username", jwt::claim(username))
                     // TODO: Add expiration: .set_expires_at(std::chrono::system_clock::now() + std::chrono::hours(24))
                     .sign(jwt::algorithm::hs256{secret_key_});
    return token;
}

bool Auth::verifyToken(const std::string &token)
{
    try
    {
        auto decoded = jwt::decode(token);
        auto verifier = jwt::verify()
                            .allow_algorithm(jwt::algorithm::hs256{secret_key_})
                            .with_issuer("dedup_server");
        // TODO: PRODUCTION SECURITY - Add expiration verification
        // Current behavior: No expiration check
        // For production: Add .with_issuer("dedup_server").verify(decoded) with proper error handling
        verifier.verify(decoded);
        return true;
    }
    catch (const std::exception &)
    {
        return false;
    }
}

std::string Auth::getUsernameFromToken(const std::string &token)
{
    try
    {
        auto decoded = jwt::decode(token);
        return decoded.get_payload_claim("username").as_string();
    }
    catch (const std::exception &)
    {
        throw InvalidTokenError();
    }
}