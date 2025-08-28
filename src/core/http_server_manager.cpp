#include "core/http_server_manager.hpp"
#include "poco_config_adapter.hpp"
#include "web/route_handlers.hpp"
#include "core/status.hpp"
#include "auth/auth.hpp"
#include "web/openapi_docs.hpp"
#include "server_config.hpp"
#include <iostream>
#include <algorithm>

HttpServerManager::HttpServerManager() : current_host_("localhost"), current_port_(8080)
{
    Logger::info("HttpServerManager: Initialized with default configuration");
}

HttpServerManager::~HttpServerManager()
{
    stop();
}

HttpServerManager& HttpServerManager::getInstance()
{
    static HttpServerManager instance;
    return instance;
}

void HttpServerManager::start(const std::string& host, int port)
{
    std::lock_guard<std::mutex> lock(server_mutex_);
    
    if (running_.load())
    {
        Logger::warn("HttpServerManager: Server is already running. Stopping current instance first.");
        stop();
    }
    
    current_host_ = host;
    current_port_ = port;
    
    // Create new server instance
    server_ = std::make_unique<httplib::Server>();
    
    // Setup routes
    setupRoutes();
    
    // Start server thread
    running_.store(true);
    server_thread_ = std::thread(&HttpServerManager::serverThread, this);
    
    Logger::info("HttpServerManager: Server started on " + host + ":" + std::to_string(port));
}

void HttpServerManager::stop()
{
    std::lock_guard<std::mutex> lock(server_mutex_);
    
    if (!running_.load())
    {
        return;
    }
    
    running_.store(false);
    
    if (server_)
    {
        server_->stop();
    }
    
    if (server_thread_.joinable())
    {
        server_thread_.join();
    }
    
    server_.reset();
    
    Logger::info("HttpServerManager: Server stopped");
}

bool HttpServerManager::isRunning() const
{
    return running_.load();
}

std::string HttpServerManager::getCurrentHost() const
{
    std::lock_guard<std::mutex> lock(config_mutex_);
    return current_host_;
}

int HttpServerManager::getCurrentPort() const
{
    std::lock_guard<std::mutex> lock(config_mutex_);
    return current_port_;
}

void HttpServerManager::setRouteSetupCallback(RouteSetupCallback callback)
{
    route_setup_callback_ = std::move(callback);
}

void HttpServerManager::onConfigUpdate(const ConfigUpdateEvent& event)
{
    // Check for server configuration changes
    bool has_server_change = false;
    std::string new_host = current_host_;
    int new_port = current_port_;
    
    // Check for server port changes
    if (std::find(event.changed_keys.begin(), event.changed_keys.end(), "server_port") != event.changed_keys.end())
    {
        try
        {
            auto& config_manager = PocoConfigAdapter::getInstance();
            new_port = config_manager.getServerPort();
            has_server_change = true;
            Logger::info("HttpServerManager: Server port configuration changed to: " + std::to_string(new_port));
        }
        catch (const std::exception& e)
        {
            Logger::error("HttpServerManager: Error reading new server port: " + std::string(e.what()));
            return;
        }
    }
    
    // Check for server host changes
    if (std::find(event.changed_keys.begin(), event.changed_keys.end(), "server_host") != event.changed_keys.end())
    {
        try
        {
            auto& config_manager = PocoConfigAdapter::getInstance();
            new_host = config_manager.getServerHost();
            has_server_change = true;
            Logger::info("HttpServerManager: Server host configuration changed to: " + new_host);
        }
        catch (const std::exception& e)
        {
            Logger::error("HttpServerManager: Error reading new server host: " + std::string(e.what()));
            return;
        }
    }
    
    // Apply server configuration changes if needed
    if (has_server_change)
    {
        if (running_.load())
        {
            Logger::info("HttpServerManager: Reconfiguring server to " + new_host + ":" + std::to_string(new_port));
            reconfigureServer(new_host, new_port);
        }
        else
        {
            Logger::info("HttpServerManager: Server not running, updating configuration for next start");
            std::lock_guard<std::mutex> lock(config_mutex_);
            current_host_ = new_host;
            current_port_ = new_port;
        }
    }
}

void HttpServerManager::serverThread()
{
    try
    {
        std::string host;
        int port;
        
        {
            std::lock_guard<std::mutex> lock(config_mutex_);
            host = current_host_;
            port = current_port_;
        }
        
        std::cout << "Server starting on http://" << host << ":" << port << std::endl;
        std::cout << "API documentation available at: http://" << host << ":" << port << ServerConfig::API_DOCS_PATH << std::endl;
        
        if (!server_->listen(host, port))
        {
            Logger::error("HttpServerManager: Failed to start server on " + host + ":" + std::to_string(port));
            running_.store(false);
            return;
        }
        
        Logger::info("HttpServerManager: Server thread completed");
    }
    catch (const std::exception& e)
    {
        Logger::error("HttpServerManager: Server thread error: " + std::string(e.what()));
        running_.store(false);
    }
}

void HttpServerManager::reconfigureServer(const std::string& new_host, int new_port)
{
    if (reconfiguring_.load())
    {
        Logger::warn("HttpServerManager: Server reconfiguration already in progress, skipping");
        return;
    }
    
    reconfiguring_.store(true);
    
    try
    {
        Logger::info("HttpServerManager: Starting server reconfiguration to " + new_host + ":" + std::to_string(new_port));
        
        // Stop current server
        if (server_)
        {
            server_->stop();
        }
        
        // Wait for server thread to finish
        if (server_thread_.joinable())
        {
            server_thread_.join();
        }
        
        // Update configuration
        {
            std::lock_guard<std::mutex> lock(config_mutex_);
            current_host_ = new_host;
            current_port_ = new_port;
        }
        
        // Create new server instance
        server_ = std::make_unique<httplib::Server>();
        
        // Setup routes
        setupRoutes();
        
        // Start new server thread
        server_thread_ = std::thread(&HttpServerManager::serverThread, this);
        
        Logger::info("HttpServerManager: Server successfully reconfigured to " + new_host + ":" + std::to_string(new_port));
        
        // Log the change for audit purposes
        Logger::info("HttpServerManager: Server configuration change audit - "
                     "Updated to " + new_host + ":" + std::to_string(new_port) + " at " +
                     std::to_string(std::chrono::system_clock::now().time_since_epoch().count()));
    }
    catch (const std::exception& e)
    {
        Logger::error("HttpServerManager: Server reconfiguration failed: " + std::string(e.what()));
        
        // Attempt to restore previous configuration
        try
        {
            if (server_)
            {
                server_->stop();
            }
            if (server_thread_.joinable())
            {
                server_thread_.join();
            }
            server_.reset();
            running_.store(false);
        }
        catch (...)
        {
            Logger::error("HttpServerManager: Failed to restore previous server state");
        }
    }
    
    reconfiguring_.store(false);
}

void HttpServerManager::setupRoutes()
{
    if (!server_)
    {
        Logger::error("HttpServerManager: Cannot setup routes - server is null");
        return;
    }
    
    try
    {
        // Get configuration for auth
        auto& config_manager = PocoConfigAdapter::getInstance();
        
        // Create status and auth objects
        Status status;
        Auth auth(config_manager.getAuthSecret());
        
        // Serve OpenAPI documentation
        server_->Get(ServerConfig::SWAGGER_JSON_PATH, [](const httplib::Request&, httplib::Response& res)
        {
            res.set_content(OpenApiDocs::getSpec(), "application/json");
        });
        
        // Serve Swagger UI
        server_->Get(ServerConfig::API_DOCS_PATH, [](const httplib::Request&, httplib::Response& res)
        {
            res.set_content(OpenApiDocs::getSwaggerUI(), "text/html");
        });
        
        // Setup routes using RouteHandlers
        RouteHandlers::setupRoutes(*server_, status, auth);
        
        Logger::info("HttpServerManager: Routes setup completed successfully");
    }
    catch (const std::exception& e)
    {
        Logger::error("HttpServerManager: Failed to setup routes: " + std::string(e.what()));
        throw;
    }
}
