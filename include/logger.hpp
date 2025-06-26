#pragma once

#include <string>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <sstream>

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
    static void log(Level level, const std::string &message)
    {
        auto now = std::chrono::system_clock::now();
        auto now_time_t = std::chrono::system_clock::to_time_t(now);
        auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                          now.time_since_epoch()) %
                      1000;

        std::stringstream ss;
        ss << std::put_time(std::localtime(&now_time_t), "%Y-%m-%d %H:%M:%S")
           << '.' << std::setfill('0') << std::setw(3) << now_ms.count()
           << " [" << levelToString(level) << "] "
           << message;

        std::cout << ss.str() << std::endl;
    }

    static const char *levelToString(Level level)
    {
        switch (level)
        {
        case Level::TRACE:
            return "TRACE";
        case Level::DEBUG:
            return "DEBUG";
        case Level::INFO:
            return "INFO ";
        case Level::WARN:
            return "WARN ";
        case Level::ERROR:
            return "ERROR";
        default:
            return "UNKNOWN";
        }
    }
};