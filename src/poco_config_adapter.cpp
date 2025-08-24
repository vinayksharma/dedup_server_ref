#include "core/poco_config_adapter.hpp"
#include "logging/logger.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include "core/config_observer.hpp"

PocoConfigAdapter::PocoConfigAdapter()
    : poco_cfg_(PocoConfigManager::getInstance())
{
    Logger::info("PocoConfigAdapter constructor called");

    // Initialize with default config from Poco
    initializeDefaultConfig();

    // Try to load existing config.json first (primary source)
    if (poco_cfg_.load("config.json"))
    {
        Logger::info("Configuration loaded from config.json (primary source)");
    }
    // Fall back to config.yaml if config.json doesn't exist
    else if (poco_cfg_.load("config.yaml"))
    {
        Logger::info("Configuration loaded from config.yaml (fallback)");
        // Save the YAML config as JSON for future use
        if (poco_cfg_.save("config.json"))
        {
            Logger::info("Migrated config.yaml to config.json");
        }
    }
    else
    {
        Logger::info("No existing configuration files found, using defaults");
        // Save the default configuration as JSON
        if (poco_cfg_.save("config.json"))
        {
            Logger::info("Created new config.json with default values");
        }
    }
}

// Configuration getters - delegate to PocoConfigManager
nlohmann::json PocoConfigAdapter::getAll() const
{
    return poco_cfg_.getAll();
}

PocoConfigAdapter::~PocoConfigAdapter()
{
    stopWatching();
}

// Configuration getters - delegate to PocoConfigManager
DedupMode PocoConfigAdapter::getDedupMode() const
{
    return poco_cfg_.getDedupMode();
}

std::string PocoConfigAdapter::getLogLevel() const
{
    return poco_cfg_.getLogLevel();
}

int PocoConfigAdapter::getServerPort() const
{
    return poco_cfg_.getServerPort();
}

std::string PocoConfigAdapter::getServerHost() const
{
    return poco_cfg_.getServerHost();
}

std::string PocoConfigAdapter::getAuthSecret() const
{
    return poco_cfg_.getAuthSecret();
}

int PocoConfigAdapter::getScanIntervalSeconds() const
{
    return poco_cfg_.getScanIntervalSeconds();
}

int PocoConfigAdapter::getProcessingIntervalSeconds() const
{
    return poco_cfg_.getProcessingIntervalSeconds();
}

// Thread configuration getters
int PocoConfigAdapter::getMaxProcessingThreads() const
{
    return poco_cfg_.getMaxProcessingThreads();
}

int PocoConfigAdapter::getMaxScanThreads() const
{
    return poco_cfg_.getMaxScanThreads();
}

std::string PocoConfigAdapter::getHttpServerThreads() const
{
    return poco_cfg_.getHttpServerThreads();
}

int PocoConfigAdapter::getDatabaseThreads() const
{
    return poco_cfg_.getDatabaseThreads();
}

// Processing configuration getters
int PocoConfigAdapter::getProcessingBatchSize() const
{
    return poco_cfg_.getProcessingBatchSize();
}

// File type configuration getters
std::map<std::string, bool> PocoConfigAdapter::getSupportedFileTypes() const
{
    return poco_cfg_.getSupportedFileTypes();
}

std::map<std::string, bool> PocoConfigAdapter::getTranscodingFileTypes() const
{
    return poco_cfg_.getTranscodingFileTypes();
}

// File type utility methods
std::vector<std::string> PocoConfigAdapter::getEnabledFileTypes() const
{
    return poco_cfg_.getEnabledFileTypes();
}

bool PocoConfigAdapter::needsTranscoding(const std::string &file_extension) const
{
    return poco_cfg_.needsTranscoding(file_extension);
}

// Category-specific enabled extensions
std::vector<std::string> PocoConfigAdapter::getEnabledImageExtensions() const
{
    return poco_cfg_.getEnabledImageExtensions();
}

std::vector<std::string> PocoConfigAdapter::getEnabledVideoExtensions() const
{
    return poco_cfg_.getEnabledVideoExtensions();
}

std::vector<std::string> PocoConfigAdapter::getEnabledAudioExtensions() const
{
    return poco_cfg_.getEnabledAudioExtensions();
}

// Cache configuration getters
uint32_t PocoConfigAdapter::getDecoderCacheSizeMB() const
{
    return poco_cfg_.getDecoderCacheSizeMB();
}

// Decoder configuration getters
int PocoConfigAdapter::getMaxDecoderThreads() const
{
    return poco_cfg_.getMaxDecoderThreads();
}

// Database configuration getters
int PocoConfigAdapter::getDatabaseMaxRetries() const
{
    return poco_cfg_.getDatabaseMaxRetries();
}

int PocoConfigAdapter::getDatabaseBackoffBaseMs() const
{
    return poco_cfg_.getDatabaseBackoffBaseMs();
}

int PocoConfigAdapter::getDatabaseMaxBackoffMs() const
{
    return poco_cfg_.getDatabaseMaxBackoffMs();
}

int PocoConfigAdapter::getDatabaseBusyTimeoutMs() const
{
    return poco_cfg_.getDatabaseBusyTimeoutMs();
}

int PocoConfigAdapter::getDatabaseOperationTimeoutMs() const
{
    return poco_cfg_.getDatabaseOperationTimeoutMs();
}

// Quality stack configuration
bool PocoConfigAdapter::getPreProcessQualityStack() const
{
    return poco_cfg_.getPreProcessQualityStack();
}

// Video processing configuration accessors
int PocoConfigAdapter::getVideoSkipDurationSeconds(DedupMode mode) const
{
    return poco_cfg_.getVideoSkipDurationSeconds(mode);
}

int PocoConfigAdapter::getVideoFramesPerSkip(DedupMode mode) const
{
    return poco_cfg_.getVideoFramesPerSkip(mode);
}

int PocoConfigAdapter::getVideoSkipCount(DedupMode mode) const
{
    return poco_cfg_.getVideoSkipCount(mode);
}

// Configuration setters with event publishing
void PocoConfigAdapter::setDedupMode(DedupMode mode)
{
    std::string old_mode = DedupModes::getModeName(getDedupMode());
    std::string new_mode = DedupModes::getModeName(mode);

    // Update Poco config
    poco_cfg_.update({{"dedup_mode", new_mode}});

    // Publish event
    ConfigEvent event;
    event.type = ConfigEventType::DEDUP_MODE_CHANGED;
    event.key = "dedup_mode";
    event.old_value = old_mode;
    event.new_value = new_mode;
    event.description = "Dedup mode changed from " + old_mode + " to " + new_mode;

    publishEvent(event);
}

void PocoConfigAdapter::setLogLevel(const std::string &level)
{
    std::string old_level = getLogLevel();

    // Update Poco config
    poco_cfg_.update({{"log_level", level}});

    // Publish event
    ConfigEvent event;
    event.type = ConfigEventType::LOG_LEVEL_CHANGED;
    event.key = "log_level";
    event.old_value = old_level;
    event.new_value = level;
    event.description = "Log level changed from " + old_level + " to " + level;

    publishEvent(event);
}

void PocoConfigAdapter::setServerPort(int port)
{
    int old_port = getServerPort();

    // Update Poco config
    poco_cfg_.update({{"server_port", port}});

    // Publish event
    ConfigEvent event;
    event.type = ConfigEventType::SERVER_PORT_CHANGED;
    event.key = "server_port";
    event.old_value = std::to_string(old_port);
    event.new_value = std::to_string(port);
    event.description = "Server port changed from " + std::to_string(old_port) + " to " + std::to_string(port);

    publishEvent(event);
}

void PocoConfigAdapter::setAuthSecret(const std::string &secret)
{
    std::string old_secret = getAuthSecret();

    // Update Poco config
    poco_cfg_.update({{"auth_secret", secret}});

    // Publish event
    ConfigEvent event;
    event.type = ConfigEventType::AUTH_SECRET_CHANGED;
    event.key = "auth_secret";
    event.old_value = old_secret;
    event.new_value = secret;
    event.description = "Auth secret changed";

    publishEvent(event);
}

void PocoConfigAdapter::updateConfig(const std::string &json_config)
{
    try
    {
        // Parse JSON and update Poco config
        auto json_obj = nlohmann::json::parse(json_config);
        poco_cfg_.update(json_obj);

        // Save to file
        poco_cfg_.save("config.json");

        // Publish general config changed event
        ConfigEvent event;
        event.type = ConfigEventType::GENERAL_CONFIG_CHANGED;
        event.key = "configuration";
        event.old_value = "";
        event.new_value = json_config;
        event.description = "Configuration updated";

        publishEvent(event);
    }
    catch (const std::exception &e)
    {
        Logger::error("Failed to update config: " + std::string(e.what()));
    }
}

// Runtime config file watching
void PocoConfigAdapter::startWatching(const std::string &file_path, int interval_seconds)
{
    if (watching_.load())
        return;

    watched_file_path_ = file_path;
    watch_interval_seconds_ = interval_seconds;

    // Initialize last write time
    try
    {
        last_write_time_ = std::filesystem::last_write_time(watched_file_path_);
    }
    catch (...)
    {
        // ignore
    }

    watching_.store(true);
    watcher_thread_ = std::thread([this]()
                                  {
        Logger::info("Starting configuration file watcher for: " + watched_file_path_);
        while (watching_.load()) {
            try {
                auto current = std::filesystem::last_write_time(watched_file_path_);
                if (last_write_time_.time_since_epoch().count() != 0 && current != last_write_time_) {
                    Logger::info("Detected change in configuration file. Reloading...");
                    
                    // Reload from Poco config
                    if (poco_cfg_.load(watched_file_path_)) {
                        // Publish reload event
                        ConfigEvent event;
                        event.type = ConfigEventType::GENERAL_CONFIG_CHANGED;
                        event.key = "configuration";
                        event.old_value = "";
                        event.new_value = "reloaded";
                        event.description = "Configuration reloaded from file";
                        
                        publishEvent(event);
                        
                        last_write_time_ = current;
                    } else {
                        Logger::warn("Failed to reload configuration from file");
                    }
                }
            } catch (const std::exception &e) {
                Logger::warn(std::string("Config watcher error: ") + e.what());
            }
            std::this_thread::sleep_for(std::chrono::seconds(watch_interval_seconds_));
        }
        Logger::info("Configuration file watcher stopped"); });
}

void PocoConfigAdapter::stopWatching()
{
    if (!watching_.load())
        return;

    watching_.store(false);
    if (watcher_thread_.joinable())
        watcher_thread_.join();
}

// Observer management
void PocoConfigAdapter::subscribe(ConfigObserver *observer)
{
    std::lock_guard<std::mutex> lock(observers_mutex_);
    observers_.push_back(observer);
    Logger::info("Configuration observer subscribed");
}

void PocoConfigAdapter::unsubscribe(ConfigObserver *observer)
{
    std::lock_guard<std::mutex> lock(observers_mutex_);
    observers_.erase(
        std::remove(observers_.begin(), observers_.end(), observer),
        observers_.end());
    Logger::info("Configuration observer unsubscribed");
}

// Configuration persistence
bool PocoConfigAdapter::loadConfig(const std::string &file_path)
{
    // If loading from YAML, migrate to JSON
    if (file_path.find(".yaml") != std::string::npos || file_path.find(".yml") != std::string::npos)
    {
        if (poco_cfg_.load(file_path))
        {
            Logger::info("Loaded configuration from " + file_path);
            // Migrate to JSON
            if (poco_cfg_.save("config.json"))
            {
                Logger::info("Migrated configuration from " + file_path + " to config.json");
            }
            return true;
        }
        return false;
    }

    return poco_cfg_.load(file_path);
}

bool PocoConfigAdapter::saveConfig(const std::string &file_path) const
{
    // If no file path specified, default to config.json
    std::string target_path = file_path.empty() ? "config.json" : file_path;
    return poco_cfg_.save(target_path);
}

// Configuration validation
bool PocoConfigAdapter::validateConfig() const
{
    return poco_cfg_.validateConfig();
}

// Processing configuration specific methods
std::string PocoConfigAdapter::getProcessingConfig() const
{
    auto json_config = poco_cfg_.getProcessingConfig();
    return json_config.dump();
}

bool PocoConfigAdapter::validateProcessingConfig() const
{
    return poco_cfg_.validateProcessingConfig();
}

void PocoConfigAdapter::updateProcessingConfig(const std::string &json_config)
{
    try
    {
        auto json_obj = nlohmann::json::parse(json_config);
        poco_cfg_.update(json_obj);

        // Save to file
        poco_cfg_.save("config.json");
    }
    catch (const std::exception &e)
    {
        Logger::error("Failed to update processing config: " + std::string(e.what()));
    }
}

// Cache configuration specific methods
std::string PocoConfigAdapter::getCacheConfig() const
{
    auto json_config = poco_cfg_.getCacheConfig();
    return json_config.dump();
}

bool PocoConfigAdapter::validateCacheConfig() const
{
    return poco_cfg_.validateCacheConfig();
}

void PocoConfigAdapter::updateCacheConfig(const std::string &json_config)
{
    try
    {
        auto json_obj = nlohmann::json::parse(json_config);
        poco_cfg_.update(json_obj);

        // Save to file
        poco_cfg_.save("config.json");
    }
    catch (const std::exception &e)
    {
        Logger::error("Failed to update cache config: " + std::string(e.what()));
    }
}

// Internal methods
void PocoConfigAdapter::publishEvent(const ConfigEvent &event)
{
    std::lock_guard<std::mutex> lock(observers_mutex_);

    // Log to stdout for immediate visibility
    std::cout << "[CONFIG CHANGE DETECTED] " << event.description << std::endl;

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

void PocoConfigAdapter::initializeDefaultConfig()
{
    // Default config is already initialized in PocoConfigManager constructor
    Logger::info("Default configuration initialized");
}
