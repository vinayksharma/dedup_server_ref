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
            res.set_content(json{{"error", "No authorization header"}}.dump(), "application/json");
            return false;
        }

        std::string token = auth_header.substr(0, 7) == "Bearer " ? auth_header.substr(7) : auth_header;

        if (!auth.verifyToken(token))
        {
            res.status = 401;
            res.set_content(json{{"error", "Invalid token"}}.dump(), "application/json");
            return false;
        }

        return true;
    }
};