#include "core/http_server_manager.hpp"

// Minimal stub implementation for tests

// Provide constructor/destructor definitions (declared in header)
HttpServerManager::HttpServerManager() = default;
HttpServerManager::~HttpServerManager() = default;

HttpServerManager &HttpServerManager::getInstance()
{
    static HttpServerManager instance;
    return instance;
}

void HttpServerManager::start(const std::string &, int)
{
    // no-op in tests
}

void HttpServerManager::stop()
{
    // no-op in tests
}

bool HttpServerManager::isRunning() const
{
    return false; // tests don't run the real server
}

void HttpServerManager::onConfigUpdate(const ConfigUpdateEvent &)
{
    // no-op in tests
}

std::string HttpServerManager::getCurrentHost() const
{
    return "localhost";
}

int HttpServerManager::getCurrentPort() const
{
    return 8080;
}

void HttpServerManager::setRouteSetupCallback(RouteSetupCallback)
{
    // no-op in tests
}
