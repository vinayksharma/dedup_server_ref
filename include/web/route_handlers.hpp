#pragma once

#include <httplib.h>
#include <nlohmann/json.hpp>
#include "core/status.hpp"
#include "core/file_utils.hpp"
#include "core/server_config_manager.hpp"
#include "auth/auth.hpp"
#include "auth/auth_middleware.hpp"
#include "logging/logger.hpp"

using json = nlohmann::json;

class RouteHandlers
{
public:
    static void setupRoutes(httplib::Server &svr, Status &status, Auth &auth)
    {
        // TODO: PRODUCTION SECURITY - Add rate limiting and request validation
        // Current behavior: No rate limiting, basic authentication only

        // Login endpoint
        svr.Post("/auth/login", [&](const httplib::Request &req, httplib::Response &res)
                 { handleLogin(req, res, auth); });

        // Status endpoint
        svr.Get("/auth/status", [&](const httplib::Request &req, httplib::Response &res)
                {
            if (!AuthMiddleware::verify_auth(req, res, auth)) return;
            handleStatus(req, res, status); });

        // Find duplicates endpoint
        svr.Post("/duplicates/find", [&](const httplib::Request &req, httplib::Response &res)
                 {
            if (!AuthMiddleware::verify_auth(req, res, auth)) return;
            handleFindDuplicates(req, res); });

        // Configuration endpoints
        svr.Get("/config", [&](const httplib::Request &req, httplib::Response &res)
                {
            if (!AuthMiddleware::verify_auth(req, res, auth)) return;
            handleGetConfig(req, res); });

        svr.Put("/config", [&](const httplib::Request &req, httplib::Response &res)
                {
            if (!AuthMiddleware::verify_auth(req, res, auth)) return;
            handleUpdateConfig(req, res); });

        svr.Post("/config/reload", [&](const httplib::Request &req, httplib::Response &res)
                 {
            if (!AuthMiddleware::verify_auth(req, res, auth)) return;
            handleReloadConfig(req, res); });

        svr.Post("/config/save", [&](const httplib::Request &req, httplib::Response &res)
                 {
            if (!AuthMiddleware::verify_auth(req, res, auth)) return;
            handleSaveConfig(req, res); });
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
                res.set_content(json{{"error", "Invalid credentials"}}.dump(), "application/json");
                return;
            }

            std::string token = auth.generateToken(username);
            res.set_content(json{{"token", token}}.dump(), "application/json");
            Logger::info("Login successful for user: " + username);
        }
        catch (const std::exception &e)
        {
            Logger::error("Login error: " + std::string(e.what()));
            res.status = 500;
            res.set_content(json{{"error", "Internal server error"}}.dump(), "application/json");
        }
    }

    static void handleStatus(const httplib::Request &req, httplib::Response &res, Status &status)
    {
        Logger::trace("Received status request");
        try
        {
            bool result = status.checkStatus();
            res.set_content(json{{"status", result}}.dump(), "application/json");
            Logger::info("Status check successful: " + std::to_string(result));
        }
        catch (const std::exception &e)
        {
            Logger::error("Status error: " + std::string(e.what()));
            res.status = 500;
            res.set_content(json{{"error", "Internal server error"}}.dump(), "application/json");
        }
    }

    static void handleFindDuplicates(const httplib::Request &req, httplib::Response &res)
    {
        Logger::trace("Received find duplicates request");
        try
        {
            auto body = json::parse(req.body);
            std::string directory = body["directory"];

            Logger::debug("Starting recursive file scan for directory: " + directory);

            // Use the existing FileUtils to scan directory recursively
            auto observable = FileUtils::listFilesAsObservable(directory, true);
            observable.subscribe(
                [](const std::string &file_path)
                {
                    // Print each file as it's found (console output)
                    std::cout << "Found file: " << file_path << std::endl;
                },
                [&res](const std::exception &e)
                {
                    Logger::error("File scan error: " + std::string(e.what()));
                    res.status = 500;
                    res.set_content(json{{"error", "Directory scan failed: " + std::string(e.what())}}.dump(), "application/json");
                },
                [&res]()
                {
                    Logger::info("Directory scan completed successfully");
                    res.set_content(json{{"message", "Directory scan completed"}}.dump(), "application/json");
                });
        }
        catch (const std::exception &e)
        {
            Logger::error("Find duplicates error: " + std::string(e.what()));
            res.status = 400;
            res.set_content(json{{"error", "Invalid request: " + std::string(e.what())}}.dump(), "application/json");
        }
    }

    // Configuration handlers
    static void handleGetConfig(const httplib::Request &req, httplib::Response &res)
    {
        Logger::trace("Received get config request");
        try
        {
            auto &config_manager = ServerConfigManager::getInstance();
            json config = config_manager.getConfig();

            res.set_content(config.dump(), "application/json");
            Logger::info("Configuration retrieved successfully");
        }
        catch (const std::exception &e)
        {
            Logger::error("Get config error: " + std::string(e.what()));
            res.status = 500;
            res.set_content(json{{"error", "Internal server error"}}.dump(), "application/json");
        }
    }

    static void handleUpdateConfig(const httplib::Request &req, httplib::Response &res)
    {
        Logger::trace("Received update config request");
        try
        {
            auto body = json::parse(req.body);
            auto &config_manager = ServerConfigManager::getInstance();

            // Validate the configuration
            if (!config_manager.validateConfig(body))
            {
                res.status = 400;
                res.set_content(json{{"error", "Invalid configuration"}}.dump(), "application/json");
                return;
            }

            // Update configuration (this will trigger reactive events)
            config_manager.updateConfig(body);

            res.set_content(json{{"message", "Configuration updated successfully"}}.dump(), "application/json");
            Logger::info("Configuration updated successfully");
        }
        catch (const std::exception &e)
        {
            Logger::error("Update config error: " + std::string(e.what()));
            res.status = 400;
            res.set_content(json{{"error", "Invalid request: " + std::string(e.what())}}.dump(), "application/json");
        }
    }

    static void handleReloadConfig(const httplib::Request &req, httplib::Response &res)
    {
        Logger::trace("Received reload config request");
        try
        {
            auto body = json::parse(req.body);
            std::string file_path = body["file_path"];

            auto &config_manager = ServerConfigManager::getInstance();

            if (config_manager.loadConfig(file_path))
            {
                res.set_content(json{{"message", "Configuration reloaded successfully"}}.dump(), "application/json");
                Logger::info("Configuration reloaded from: " + file_path);
            }
            else
            {
                res.status = 400;
                res.set_content(json{{"error", "Failed to reload configuration"}}.dump(), "application/json");
            }
        }
        catch (const std::exception &e)
        {
            Logger::error("Reload config error: " + std::string(e.what()));
            res.status = 400;
            res.set_content(json{{"error", "Invalid request: " + std::string(e.what())}}.dump(), "application/json");
        }
    }

    static void handleSaveConfig(const httplib::Request &req, httplib::Response &res)
    {
        Logger::trace("Received save config request");
        try
        {
            auto body = json::parse(req.body);
            std::string file_path = body["file_path"];

            auto &config_manager = ServerConfigManager::getInstance();

            if (config_manager.saveConfig(file_path))
            {
                res.set_content(json{{"message", "Configuration saved successfully"}}.dump(), "application/json");
                Logger::info("Configuration saved to: " + file_path);
            }
            else
            {
                res.status = 500;
                res.set_content(json{{"error", "Failed to save configuration"}}.dump(), "application/json");
            }
        }
        catch (const std::exception &e)
        {
            Logger::error("Save config error: " + std::string(e.what()));
            res.status = 400;
            res.set_content(json{{"error", "Invalid request: " + std::string(e.what())}}.dump(), "application/json");
        }
    }
};