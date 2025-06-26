#pragma once

#include <string>
#include "server_config.hpp"

class OpenApiDocs
{
public:
  static const std::string &getSpec()
  {
    static const std::string spec = R"({
  "openapi": "3.0.0",
  "info": {
    "title": "Dedup API",
    "version": "1.0.0",
    "description": "A secure dedup service with JWT authentication"
  },
  "servers": [
    {
      "url": ")" + ServerConfig::getServerUrl() +
                                    R"(",
      "description": "Local development server"
    }
  ],
  "components": {
    "securitySchemes": {
      "bearerAuth": {
        "type": "http",
        "scheme": "bearer",
        "bearerFormat": "JWT"
      }
    }
  },
  "security": [
    {
      "bearerAuth": []
    }
  ],
  "paths": {
    "/auth/login": {
      "post": {
        "summary": "Login to get JWT token",
        "requestBody": {
          "required": true,
          "content": {
            "application/json": {
              "schema": {
                "type": "object",
                "properties": {
                  "username": {
                    "type": "string"
                  },
                  "password": {
                    "type": "string"
                  }
                },
                "required": ["username", "password"]
              }
            }
          }
        },
        "responses": {
          "200": {
            "description": "Successful login",
            "content": {
              "application/json": {
                "schema": {
                  "type": "object",
                  "properties": {
                    "token": {
                      "type": "string"
                    }
                  }
                }
              }
            }
          },
          "401": {
            "description": "Invalid credentials"
          }
        }
      }
    },
    "/auth/status": {
      "get": {
        "summary": "Check service status",
        "responses": {
          "200": {
            "description": "Service status",
            "content": {
              "application/json": {
                "schema": {
                  "type": "object",
                  "properties": {
                    "status": {
                      "type": "boolean"
                    }
                  }
                }
              }
            }
          },
          "401": {
            "description": "Unauthorized"
          },
          "500": {
            "description": "Internal server error"
          }
        }
      }
    }
  }
})";
    return spec;
  }

  static std::string getSwaggerUI()
  {
    return R"(
            <!DOCTYPE html>
            <html>
            <head>
                <title>Dedup API Documentation</title>
                <link rel="stylesheet" type="text/css" href="https://unpkg.com/swagger-ui-dist@4/swagger-ui.css">
                <script src="https://unpkg.com/swagger-ui-dist@4/swagger-ui-bundle.js"></script>
            </head>
            <body>
                <div id="swagger-ui"></div>
                <script>
                    window.onload = function() {
                        SwaggerUIBundle({
                            url: ")" +
           std::string(ServerConfig::SWAGGER_JSON_PATH) + R"(",
                            dom_id: '#swagger-ui',
                            deepLinking: true,
                            presets: [
                                SwaggerUIBundle.presets.apis,
                                SwaggerUIBundle.SwaggerUIStandalonePreset
                            ],
                        });
                    };
                </script>
            </body>
            </html>
        )";
  }
};