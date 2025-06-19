#pragma once

#include <httplib.h>
#include <string>
#include <memory>

class ServerConfig
{
public:
    static constexpr const char *HOST = "localhost";
    static constexpr int PORT = 8080;
    static constexpr const char *API_DOCS_PATH = "/docs";
    static constexpr const char *SWAGGER_JSON_PATH = "/swagger.json";

    static std::string getServerUrl()
    {
        return std::string("http://") + HOST + ":" + std::to_string(PORT);
    }
};