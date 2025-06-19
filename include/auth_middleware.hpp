#pragma once

#include <httplib.h>
#include <nlohmann/json.hpp>
#include "auth.hpp"

using json = nlohmann::json;

class AuthMiddleware
{
public:
    static bool verify_auth(const httplib::Request &req, httplib::Response &res, Auth &auth)
    {
        auto auth_header = req.get_header_value("Authorization");
        if (auth_header.empty())
        {
            res.status = 401;
            json error = {{"error", "No authorization header"}};
            res.set_content(error.dump(), "application/json");
            return false;
        }

        // Remove "Bearer " prefix if present
        std::string token = auth_header;
        if (token.substr(0, 7) == "Bearer ")
        {
            token = token.substr(7);
        }

        if (!auth.verifyToken(token))
        {
            res.status = 401;
            json error = {{"error", "Invalid token"}};
            res.set_content(error.dump(), "application/json");
            return false;
        }

        return true;
    }
};