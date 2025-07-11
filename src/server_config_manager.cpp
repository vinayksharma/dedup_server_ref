#include "core/server_config_manager.hpp"
#include "logging/logger.hpp"
#include <fstream>
#include <iostream>

ServerConfigManager::ServerConfigManager()
{
    initializeDefaultConfig();
}

ServerConfigManager &ServerConfigManager::getInstance()
{
    static ServerConfigManager instance;
    return instance;
}

void ServerConfigManager::initializeDefaultConfig()
{
    std::lock_guard<std::mutex> lock(config_mutex_);

    config_ = {
        {"dedup_mode", "BALANCED"},
        {"log_level", "INFO"},
        {"server_port", 8080},
        {"auth_secret", "your-secret-key-here"},
        {"server_host", "localhost"},
        {"max_file_size", 1000000000}, // 1GB
        {"scan_timeout", 300}          // 5 minutes
    };
}

DedupMode ServerConfigManager::getDedupMode() const
{
    std::lock_guard<std::mutex> lock(config_mutex_);
    std::string mode_str = config_["dedup_mode"];
    return DedupModes::fromString(mode_str);
}

std::string ServerConfigManager::getLogLevel() const
{
    std::lock_guard<std::mutex> lock(config_mutex_);
    return config_["log_level"];
}

int ServerConfigManager::getServerPort() const
{
    std::lock_guard<std::mutex> lock(config_mutex_);
    return config_["server_port"];
}

std::string ServerConfigManager::getAuthSecret() const
{
    std::lock_guard<std::mutex> lock(config_mutex_);
    return config_["auth_secret"];
}

json ServerConfigManager::getConfig() const
{
    std::lock_guard<std::mutex> lock(config_mutex_);
    return config_;
}

void ServerConfigManager::setDedupMode(DedupMode mode)
{
    std::lock_guard<std::mutex> lock(config_mutex_);

    std::string old_mode = config_["dedup_mode"];
    std::string new_mode = DedupModes::getModeName(mode);

    if (old_mode != new_mode)
    {
        json old_value = old_mode;
        json new_value = new_mode;

        config_["dedup_mode"] = new_mode;

        ConfigEvent event{
            ConfigEventType::DEDUP_MODE_CHANGED,
            "dedup_mode",
            old_value,
            new_value,
            "Deduplication mode changed from " + old_mode + " to " + new_mode};

        publishEvent(event);
    }
}

void ServerConfigManager::setLogLevel(const std::string &level)
{
    std::lock_guard<std::mutex> lock(config_mutex_);

    std::string old_level = config_["log_level"];

    if (old_level != level)
    {
        json old_value = old_level;
        json new_value = level;

        config_["log_level"] = level;

        ConfigEvent event{
            ConfigEventType::LOG_LEVEL_CHANGED,
            "log_level",
            old_value,
            new_value,
            "Log level changed from " + old_level + " to " + level};

        publishEvent(event);
    }
}

void ServerConfigManager::setServerPort(int port)
{
    std::lock_guard<std::mutex> lock(config_mutex_);

    int old_port = config_["server_port"];

    if (old_port != port)
    {
        json old_value = old_port;
        json new_value = port;

        config_["server_port"] = port;

        ConfigEvent event{
            ConfigEventType::SERVER_PORT_CHANGED,
            "server_port",
            old_value,
            new_value,
            "Server port changed from " + std::to_string(old_port) + " to " + std::to_string(port)};

        publishEvent(event);
    }
}

void ServerConfigManager::setAuthSecret(const std::string &secret)
{
    std::lock_guard<std::mutex> lock(config_mutex_);

    std::string old_secret = config_["auth_secret"];

    if (old_secret != secret)
    {
        json old_value = old_secret;
        json new_value = secret;

        config_["auth_secret"] = secret;

        ConfigEvent event{
            ConfigEventType::AUTH_SECRET_CHANGED,
            "auth_secret",
            old_value,
            new_value,
            "Auth secret changed"};

        publishEvent(event);
    }
}

void ServerConfigManager::updateConfig(const json &new_config)
{
    std::lock_guard<std::mutex> lock(config_mutex_);

    json old_config = config_;

    // Merge new config with existing config
    for (auto it = new_config.begin(); it != new_config.end(); ++it)
    {
        if (config_.contains(it.key()))
        {
            json old_value = config_[it.key()];
            json new_value = it.value();

            if (old_value != new_value)
            {
                config_[it.key()] = it.value();

                ConfigEvent event{
                    ConfigEventType::GENERAL_CONFIG_CHANGED,
                    it.key(),
                    old_value,
                    new_value,
                    "Configuration key '" + it.key() + "' updated"};

                publishEvent(event);
            }
        }
        else
        {
            config_[it.key()] = it.value();
        }
    }
}

void ServerConfigManager::subscribe(ConfigObserver *observer)
{
    std::lock_guard<std::mutex> lock(observers_mutex_);
    observers_.push_back(observer);
    Logger::info("Configuration observer subscribed");
}

void ServerConfigManager::unsubscribe(ConfigObserver *observer)
{
    std::lock_guard<std::mutex> lock(observers_mutex_);
    observers_.erase(
        std::remove(observers_.begin(), observers_.end(), observer),
        observers_.end());
    Logger::info("Configuration observer unsubscribed");
}

void ServerConfigManager::publishEvent(const ConfigEvent &event)
{
    std::lock_guard<std::mutex> lock(observers_mutex_);

    Logger::info("Publishing config event: " + event.description);

    for (auto observer : observers_)
    {
        try
        {
            observer->onConfigChanged(event);
        }
        catch (const std::exception &e)
        {
            Logger::error("Error in config observer: " + std::string(e.what()));
        }
    }
}

bool ServerConfigManager::loadConfig(const std::string &file_path)
{
    try
    {
        std::ifstream file(file_path);
        if (!file.is_open())
        {
            Logger::error("Could not open config file: " + file_path);
            return false;
        }

        json file_config;
        file >> file_config;

        if (validateConfig(file_config))
        {
            updateConfig(file_config);
            Logger::info("Configuration loaded from: " + file_path);
            return true;
        }
        else
        {
            Logger::error("Invalid configuration in file: " + file_path);
            return false;
        }
    }
    catch (const std::exception &e)
    {
        Logger::error("Error loading config: " + std::string(e.what()));
        return false;
    }
}

bool ServerConfigManager::saveConfig(const std::string &file_path) const
{
    try
    {
        std::ofstream file(file_path);
        if (!file.is_open())
        {
            Logger::error("Could not open config file for writing: " + file_path);
            return false;
        }

        std::lock_guard<std::mutex> lock(config_mutex_);
        file << config_.dump(4);

        Logger::info("Configuration saved to: " + file_path);
        return true;
    }
    catch (const std::exception &e)
    {
        Logger::error("Error saving config: " + std::string(e.what()));
        return false;
    }
}

bool ServerConfigManager::validateConfig(const json &config) const
{
    // Basic validation - check for required fields
    std::vector<std::string> required_fields = {
        "dedup_mode", "log_level", "server_port", "auth_secret", "max_file_size", "scan_timeout"};

    for (const auto &field : required_fields)
    {
        if (!config.contains(field))
        {
            Logger::error("Missing required config field: " + field);
            return false;
        }
    }

    // Validate specific fields
    if (config["server_port"].get<int>() <= 0 || config["server_port"].get<int>() > 65535)
    {
        Logger::error("Invalid server port: " + std::to_string(config["server_port"].get<int>()));
        return false;
    }

    std::string mode = config["dedup_mode"];
    if (mode != "FAST" && mode != "BALANCED" && mode != "QUALITY")
    {
        Logger::error("Invalid dedup mode: " + mode);
        return false;
    }

    if (config["max_file_size"].get<int>() <= 0)
    {
        Logger::error("Invalid max_file_size: " + std::to_string(config["max_file_size"].get<int>()));
        return false;
    }

    if (config["scan_timeout"].get<int>() <= 0)
    {
        Logger::error("Invalid scan_timeout: " + std::to_string(config["scan_timeout"].get<int>()));
        return false;
    }

    return true;
}