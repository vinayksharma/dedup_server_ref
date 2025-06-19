#include "auth.hpp"
#include <chrono>
#include <stdexcept>

std::string Auth::authenticate(const std::string &username, const std::string &password)
{
    // TODO: Implement actual user authentication
    if (username == "admin" && password == "password")
    {
        return generateToken(username);
    }
    throw InvalidCredentialsError();
}

std::string Auth::generateToken(const std::string &username)
{
    auto token = jwt::create()
                     .set_issuer("dedup_server")
                     .set_type("JWS")
                     .set_payload_claim("username", jwt::claim(username))
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