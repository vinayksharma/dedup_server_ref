#pragma once

#include <httplib.h>
#include <nlohmann/json.hpp>
#include "status.hpp"
#include "auth.hpp"
#include "auth_middleware.hpp"
#include "logger.hpp"

using json = nlohmann::json;

class RouteHandlers
{
public:
    static void setupRoutes(httplib::Server &svr, Status &status, Auth &auth)
    {
        // Login endpoint
        svr.Post("/login", [&](const httplib::Request &req, httplib::Response &res)
                 { handleLogin(req, res, auth); });

        // Status endpoint
        svr.Get("/status", [&](const httplib::Request &req, httplib::Response &res)
                {
            if (!AuthMiddleware::verify_auth(req, res, auth)) return;
            handleStatus(req, res, status); });
    }

private:
    static void handleLogin(const httplib::Request &req, httplib::Response &res, Auth &auth)
    {
        Logger::trace("Received login request");
        try
        {
            auto body = json::parse(req.body);
            std::string username = body["username"];
            std::string password = body["password"];

            Logger::debug("Attempting login for user: " + username);
            if (username.empty() || password.empty())
            {
                Logger::warn("Login failed: Invalid credentials");
                res.status = 401;
                json error = {{"error", "Invalid credentials"}};
                res.set_content(error.dump(), "application/json");
                return;
            }

            std::string token = auth.generateToken(username);
            json response = {{"token", token}};
            res.set_content(response.dump(), "application/json");
            Logger::info("Login successful for user: " + username);
        }
        catch (const Auth::InvalidCredentialsError &e)
        {
            Logger::warn("Login failed: Invalid credentials");
            res.status = 401;
            json error = {{"error", "Invalid credentials"}};
            res.set_content(error.dump(), "application/json");
        }
        catch (const std::exception &e)
        {
            Logger::error("Login error: " + std::string(e.what()));
            res.status = 500;
            json error = {{"error", "Internal server error"}};
            res.set_content(error.dump(), "application/json");
        }
    }

    static void handleStatus(const httplib::Request &req, httplib::Response &res, Status &status)
    {
        Logger::trace("Received status request");
        try
        {
            bool result = status.checkStatus();
            json response = {{"status", result}};
            res.set_content(response.dump(), "application/json");
            Logger::info("Status check successful: " + std::to_string(result));
        }
        catch (const std::exception &e)
        {
            Logger::error("Status error: " + std::string(e.what()));
            res.status = 500;
            json error = {{"error", "Internal server error"}};
            res.set_content(error.dump(), "application/json");
        }
    }
};