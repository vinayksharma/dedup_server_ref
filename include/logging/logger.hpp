#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <memory>

class Logger
{
public:
    enum class Level
    {
        TRACE,
        DEBUG,
        INFO,
        WARN,
        ERROR
    };

    static void trace(const std::string &message)
    {
        log(Level::TRACE, message);
    }

    static void debug(const std::string &message)
    {
        log(Level::DEBUG, message);
    }

    static void info(const std::string &message)
    {
        log(Level::INFO, message);
    }

    static void warn(const std::string &message)
    {
        log(Level::WARN, message);
    }

    static void error(const std::string &message)
    {
        log(Level::ERROR, message);
    }

private:
    static std::shared_ptr<spdlog::logger> getLogger()
    {
        static auto logger = spdlog::stdout_color_mt("dedup_server");
        return logger;
    }

    static void log(Level level, const std::string &message)
    {
        auto logger = getLogger();
        switch (level)
        {
        case Level::TRACE:
            logger->trace(message);
            break;
        case Level::DEBUG:
            logger->debug(message);
            break;
        case Level::INFO:
            logger->info(message);
            break;
        case Level::WARN:
            logger->warn(message);
            break;
        case Level::ERROR:
            logger->error(message);
            break;
        }
    }
};