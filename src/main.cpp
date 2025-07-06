#include "core/status.hpp"
#include "auth/auth.hpp"
#include "core/server_config.hpp"
#include "auth/auth_middleware.hpp"
#include "web/route_handlers.hpp"
#include "web/openapi_docs.hpp"
#include <httplib.h>
#include <iostream>
#include <jwt-cpp/jwt.h>

int main()
{
        Status status;
        // TODO: PRODUCTION SECURITY - Replace hardcoded secret key with environment variable
        // Current behavior: Tokens persist across server restarts because secret key is hardcoded
        // For production: Use environment variable like getenv("JWT_SECRET_KEY")
        Auth auth("your-secret-key-here"); // In production, use a secure key from environment variable

        httplib::Server svr;

        // Serve OpenAPI documentation
        svr.Get(ServerConfig::SWAGGER_JSON_PATH, [](const httplib::Request &, httplib::Response &res)
                { res.set_content(OpenApiDocs::getSpec(), "application/json"); });

        // Serve Swagger UI
        svr.Get(ServerConfig::API_DOCS_PATH, [](const httplib::Request &, httplib::Response &res)
                { res.set_content(OpenApiDocs::getSwaggerUI(), "text/html"); });

        // Setup routes
        RouteHandlers::setupRoutes(svr, status, auth);

        std::cout << "Server starting on " << ServerConfig::getServerUrl() << std::endl;
        std::cout << "API documentation available at: " << ServerConfig::getServerUrl() << ServerConfig::API_DOCS_PATH << std::endl;
        svr.listen(ServerConfig::HOST, ServerConfig::PORT);
        return 0;
}