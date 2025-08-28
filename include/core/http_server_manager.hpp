#pragma once

#include <httplib.h>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <functional>
#include <string>
#include "config_observer.hpp"
#include "logging/logger.hpp"

/**
 * @brief HTTP Server Manager for dynamic configuration changes
 * 
 * This class manages the HTTP server lifecycle and allows dynamic reconfiguration
 * of server_port and server_host without requiring a full application restart.
 * It implements ConfigObserver to react to configuration changes in real-time.
 */
class HttpServerManager : public ConfigObserver
{
public:
    // Singleton pattern
    static HttpServerManager& getInstance();
    
    // Server lifecycle
    void start(const std::string& host, int port);
    void stop();
    bool isRunning() const;
    
    // Configuration change handlers
    void onConfigUpdate(const ConfigUpdateEvent& event) override;
    
    // Server access for route setup
    httplib::Server* getServer() { return server_.get(); }
    
    // Current configuration
    std::string getCurrentHost() const;
    int getCurrentPort() const;
    
    // Route setup callback
    using RouteSetupCallback = std::function<void(httplib::Server&)>;
    void setRouteSetupCallback(RouteSetupCallback callback);

private:
    HttpServerManager();
    ~HttpServerManager();
    HttpServerManager(const HttpServerManager&) = delete;
    HttpServerManager& operator=(const HttpServerManager&) = delete;
    
    // Internal methods
    void serverThread();
    void reconfigureServer(const std::string& new_host, int new_port);
    void setupRoutes();
    
    // Member variables
    std::unique_ptr<httplib::Server> server_;
    std::thread server_thread_;
    std::atomic<bool> running_{false};
    std::atomic<bool> reconfiguring_{false};
    
    // Current configuration
    std::string current_host_;
    int current_port_;
    
    // Route setup callback
    RouteSetupCallback route_setup_callback_;
    
    // Thread safety
    mutable std::mutex server_mutex_;
    mutable std::mutex config_mutex_;
};
