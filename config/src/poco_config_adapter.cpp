#include "poco_config_adapter.hpp"
#include "logging/logger.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <atomic>
#include "config_observer.hpp"

PocoConfigAdapter::PocoConfigAdapter()
    : poco_cfg_(PocoConfigManager::getInstance())
{
    Logger::info("PocoConfigAdapter constructor called");

    // Initialize with default config from Poco
    initializeDefaultConfig();

    // Try to load configuration from common locations (project and build dirs)
    bool loaded = false;
    const std::vector<std::string> candidate_paths = {
        "config/config.json",    // prefer project config first
        "../config/config.json", // running from build/
        "config.json"            // last resort: local working dir
    };

    for (const auto &path : candidate_paths)
    {
        if (poco_cfg_.load(path))
        {
            Logger::info("Configuration loaded from " + path + " (primary source)");
            loaded = true;
            break;
        }
    }

    // Fall back to legacy YAML if present (unlikely)
    if (!loaded && poco_cfg_.load("config.yaml"))
    {
        Logger::info("Configuration loaded from config.yaml (fallback)");
        // Save the YAML config as JSON for future use
        if (poco_cfg_.save("config.json"))
        {
            Logger::info("Migrated config.yaml to config.json");
        }
        loaded = true;
    }

    if (!loaded)
    {
        Logger::info("No existing configuration files found, using defaults");
        // Save the default configuration as JSON in current dir to make it visible
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

    // Persist changes to config.json
    persistChanges("dedup_mode");

    // Publish event
    ConfigUpdateEvent event;
    event.changed_keys = {"dedup_mode"};
    event.source = "api";
    event.update_id = generateUpdateId();

    publishEvent(event);
}

void PocoConfigAdapter::setLogLevel(const std::string &level)
{
    std::string old_level = getLogLevel();

    // Update Poco config
    poco_cfg_.update({{"log_level", level}});

    // Persist changes to config.json
    persistChanges("log_level");

    // Publish event
    ConfigUpdateEvent event;
    event.changed_keys = {"log_level"};
    event.source = "api";
    event.update_id = generateUpdateId();

    publishEvent(event);
}

void PocoConfigAdapter::setServerPort(int port)
{
    int old_port = getServerPort();

    // Update Poco config
    poco_cfg_.update({{"server_port", port}});

    // Persist changes to config.json
    persistChanges("server_port");

    // Publish event
    ConfigUpdateEvent event;
    event.changed_keys = {"server_port"};
    event.source = "api";
    event.update_id = generateUpdateId();

    publishEvent(event);
}

void PocoConfigAdapter::setAuthSecret(const std::string &secret)
{
    std::string old_secret = getAuthSecret();

    // Update Poco config
    poco_cfg_.update({{"auth_secret", secret}});

    // Persist changes to config.json
    persistChanges("auth_secret");

    // Publish event
    ConfigUpdateEvent event;
    event.changed_keys = {"auth_secret"};
    event.source = "api";
    event.update_id = generateUpdateId();

    publishEvent(event);
}

void PocoConfigAdapter::setServerHost(const std::string &host)
{
    // Update Poco config
    poco_cfg_.update({{"server_host", host}});

    // Persist changes to config.json
    persistChanges("server_host");

    // Publish event
    ConfigUpdateEvent event;
    event.changed_keys = {"server_host"};
    event.source = "api";
    event.update_id = generateUpdateId();

    publishEvent(event);
}

void PocoConfigAdapter::setScanIntervalSeconds(int seconds)
{
    // Update Poco config
    poco_cfg_.update({{"scan_interval_seconds", seconds}});

    // Persist changes to config.json
    persistChanges("scan_interval_seconds");

    // Publish event
    ConfigUpdateEvent event;
    event.changed_keys = {"scan_interval_seconds"};
    event.source = "api";
    event.update_id = generateUpdateId();

    publishEvent(event);
}

void PocoConfigAdapter::setProcessingIntervalSeconds(int seconds)
{
    // Update Poco config
    poco_cfg_.update({{"processing_interval_seconds", seconds}});

    // Persist changes to config.json
    persistChanges("processing_interval_seconds");

    // Publish event
    ConfigUpdateEvent event;
    event.changed_keys = {"processing_interval_seconds"};
    event.source = "api";
    event.update_id = generateUpdateId();

    publishEvent(event);
}

void PocoConfigAdapter::setMaxProcessingThreads(int threads)
{
    // Update Poco config
    poco_cfg_.update({{"max_processing_threads", threads}});

    // Persist changes to config.json
    persistChanges("max_processing_threads");

    // Publish event
    ConfigUpdateEvent event;
    event.changed_keys = {"max_processing_threads"};
    event.source = "api";
    event.update_id = generateUpdateId();

    publishEvent(event);
}

void PocoConfigAdapter::setMaxScanThreads(int threads)
{
    // Update Poco config
    poco_cfg_.update({{"max_scan_threads", threads}});

    // Persist changes to config.json
    persistChanges("max_scan_threads");

    // Publish event
    ConfigUpdateEvent event;
    event.changed_keys = {"max_scan_threads"};
    event.source = "api";
    event.update_id = generateUpdateId();

    publishEvent(event);
}

void PocoConfigAdapter::setHttpServerThreads(const std::string &threads)
{
    // Update Poco config
    poco_cfg_.update({{"http_server_threads", threads}});

    // Persist changes to config.json
    persistChanges("http_server_threads");

    // Publish event
    ConfigUpdateEvent event;
    event.changed_keys = {"http_server_threads"};
    event.source = "api";
    event.update_id = generateUpdateId();

    publishEvent(event);
}

void PocoConfigAdapter::setDatabaseThreads(int threads)
{
    // Update Poco config
    poco_cfg_.update({{"database_threads", threads}});

    // Persist changes to config.json
    persistChanges("database_threads");

    // Publish event
    ConfigUpdateEvent event;
    event.changed_keys = {"database_threads"};
    event.source = "api";
    event.update_id = generateUpdateId();

    publishEvent(event);
}

void PocoConfigAdapter::setMaxDecoderThreads(int threads)
{
    // Update Poco config
    poco_cfg_.update({{"max_decoder_threads", threads}});

    // Persist changes to config.json
    persistChanges("max_decoder_threads");

    // Publish event
    ConfigUpdateEvent event;
    event.changed_keys = {"max_decoder_threads"};
    event.source = "api";
    event.update_id = generateUpdateId();

    publishEvent(event);
}

void PocoConfigAdapter::setProcessingBatchSize(int batch_size)
{
    // Update Poco config
    poco_cfg_.update({{"processing_batch_size", batch_size}});

    // Persist changes to config.json
    persistChanges("processing_batch_size");

    // Publish event
    ConfigUpdateEvent event;
    event.changed_keys = {"processing_batch_size"};
    event.source = "api";
    event.update_id = generateUpdateId();

    publishEvent(event);
}

void PocoConfigAdapter::setPreProcessQualityStack(bool enabled)
{
    // Update Poco config
    poco_cfg_.update({{"pre_process_quality_stack", enabled}});

    // Persist changes to config.json
    persistChanges("pre_process_quality_stack");

    // Publish event
    ConfigUpdateEvent event;
    event.changed_keys = {"pre_process_quality_stack"};
    event.source = "api";
    event.update_id = generateUpdateId();

    publishEvent(event);
}

void PocoConfigAdapter::setDecoderCacheSizeMB(uint32_t size_mb)
{
    // Update Poco config
    poco_cfg_.update({{"decoder_cache_size_mb", size_mb}});

    // Persist changes to config.json
    persistChanges("decoder_cache_size_mb");

    // Publish event
    ConfigUpdateEvent event;
    event.changed_keys = {"decoder_cache_size_mb"};
    event.source = "api";
    event.update_id = generateUpdateId();

    publishEvent(event);
}

void PocoConfigAdapter::setDatabaseMaxRetries(int max_retries)
{
    // Update Poco config
    poco_cfg_.update({{"database_max_retries", max_retries}});

    // Persist changes to config.json
    persistChanges("database_max_retries");

    // Publish event
    ConfigUpdateEvent event;
    event.changed_keys = {"database_max_retries"};
    event.source = "api";
    event.update_id = generateUpdateId();

    publishEvent(event);
}

void PocoConfigAdapter::setDatabaseBackoffBaseMs(int backoff_ms)
{
    // Update Poco config
    poco_cfg_.update({{"database_backoff_base_ms", backoff_ms}});

    // Persist changes to config.json
    persistChanges("database_backoff_base_ms");

    // Publish event
    ConfigUpdateEvent event;
    event.changed_keys = {"database_backoff_base_ms"};
    event.source = "api";
    event.update_id = generateUpdateId();

    publishEvent(event);
}

void PocoConfigAdapter::setDatabaseMaxBackoffMs(int max_backoff_ms)
{
    // Update Poco config
    poco_cfg_.update({{"database_max_backoff_ms", max_backoff_ms}});

    // Persist changes to config.json
    persistChanges("database_max_backoff_ms");

    // Publish event
    ConfigUpdateEvent event;
    event.changed_keys = {"database_max_backoff_ms"};
    event.source = "api";
    event.update_id = generateUpdateId();

    publishEvent(event);
}

void PocoConfigAdapter::setDatabaseBusyTimeoutMs(int timeout_ms)
{
    // Update Poco config
    poco_cfg_.update({{"database_busy_timeout_ms", timeout_ms}});

    // Persist changes to config.json
    persistChanges("database_busy_timeout_ms");

    // Publish event
    ConfigUpdateEvent event;
    event.changed_keys = {"database_busy_timeout_ms"};
    event.source = "api";
    event.update_id = generateUpdateId();

    publishEvent(event);
}

void PocoConfigAdapter::setDatabaseOperationTimeoutMs(int timeout_ms)
{
    // Update Poco config
    poco_cfg_.update({{"database_operation_timeout_ms", timeout_ms}});

    // Persist changes to config.json
    persistChanges("database_operation_timeout_ms");

    // Publish event
    ConfigUpdateEvent event;
    event.changed_keys = {"database_operation_timeout_ms"};
    event.source = "api";
    event.update_id = generateUpdateId();

    publishEvent(event);
}

void PocoConfigAdapter::setFileTypeEnabled(const std::string &category, const std::string &extension, bool enabled)
{
    // Create JSON patch for the specific category.extension
    nlohmann::json patch;
    patch["categories"][category][extension] = enabled;

    poco_cfg_.update(patch);
    persistChanges("categories." + category + "." + extension);

    // Publish configuration change event
    std::vector<std::string> changed_keys = {"categories." + category + "." + extension};
    ConfigUpdateEvent event;
    event.changed_keys = changed_keys;
    event.source = "setFileTypeEnabled";
    event.update_id = generateUpdateId();
    publishEvent(event);
}

void PocoConfigAdapter::setTranscodingFileType(const std::string &extension, bool enabled)
{
    // Create JSON patch for transcoding extension
    nlohmann::json patch;
    patch["transcoding"][extension] = enabled;

    poco_cfg_.update(patch);
    persistChanges("transcoding." + extension);

    // Publish configuration change event
    std::vector<std::string> changed_keys = {"transcoding." + extension};
    ConfigUpdateEvent event;
    event.changed_keys = changed_keys;
    event.source = "setTranscodingFileType";
    event.update_id = generateUpdateId();
    publishEvent(event);
}

void PocoConfigAdapter::updateFileTypeConfig(const std::string &category, const std::string &extension, bool enabled)
{
    // Create JSON patch for the specific category.extension
    nlohmann::json patch;
    patch["categories"][category][extension] = enabled;

    poco_cfg_.update(patch);
    persistChanges("categories." + category + "." + extension);

    // Publish configuration change event
    std::vector<std::string> changed_keys = {"categories." + category + "." + extension};
    ConfigUpdateEvent event;
    event.changed_keys = changed_keys;
    event.source = "updateFileTypeConfig";
    event.update_id = generateUpdateId();
    publishEvent(event);
}

void PocoConfigAdapter::setVideoSkipDurationSeconds(int seconds)
{
    // Create JSON patch for video skip duration
    nlohmann::json patch;
    patch["video"]["skip_duration_seconds"] = seconds;

    poco_cfg_.update(patch);
    persistChanges("video.skip_duration_seconds");

    // Publish configuration change event
    std::vector<std::string> changed_keys = {"video.skip_duration_seconds"};
    ConfigUpdateEvent event;
    event.changed_keys = changed_keys;
    event.source = "setVideoSkipDurationSeconds";
    event.update_id = generateUpdateId();
    publishEvent(event);
}

void PocoConfigAdapter::setVideoFramesPerSkip(int frames)
{
    // Create JSON patch for video frames per skip
    nlohmann::json patch;
    patch["video"]["frames_per_skip"] = frames;

    poco_cfg_.update(patch);
    persistChanges("video.frames_per_skip");

    // Publish configuration change event
    std::vector<std::string> changed_keys = {"video.frames_per_skip"};
    ConfigUpdateEvent event;
    event.changed_keys = changed_keys;
    event.source = "setVideoFramesPerSkip";
    event.update_id = generateUpdateId();
    publishEvent(event);
}

void PocoConfigAdapter::setVideoSkipCount(int count)
{
    // Create JSON patch for video skip count
    nlohmann::json patch;
    patch["video"]["skip_count"] = count;

    poco_cfg_.update(patch);
    persistChanges("video.skip_count");

    // Publish configuration change event
    std::vector<std::string> changed_keys = {"video.skip_count"};
    ConfigUpdateEvent event;
    event.changed_keys = changed_keys;
    event.source = "setVideoSkipCount";
    event.update_id = generateUpdateId();
    publishEvent(event);
}

// Remove bulk update method - keeping only specific setters
// void PocoConfigAdapter::updateConfigAndPersist(const nlohmann::json &config_updates, const std::vector<std::string> &changed_keys) {
//     // Implementation removed for cleaner API
// }

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
        ConfigUpdateEvent event;
        event.changed_keys = {"configuration"};
        event.source = "api";
        event.update_id = generateUpdateId();

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
                        ConfigUpdateEvent event;
                        event.changed_keys = {"configuration"};
                        event.source = "file_observer";
                        event.update_id = generateUpdateId();
                        
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

    // Try provided path and common fallbacks relative to current working dir
    const std::vector<std::string> candidate_paths = {
        file_path,
        std::string("../") + file_path,
        "config/config.json",
        "../config/config.json",
        "config.json"};

    for (const auto &path : candidate_paths)
    {
        if (poco_cfg_.load(path))
        {
            Logger::info("Loaded configuration from " + path);
            return true;
        }
    }

    return false;
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

        // Publish event
        ConfigUpdateEvent event;
        event.changed_keys = {"processing_config"};
        event.source = "api";
        event.update_id = generateUpdateId();

        publishEvent(event);
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

        // Publish event
        ConfigUpdateEvent event;
        event.changed_keys = {"cache_config"};
        event.source = "api";
        event.update_id = generateUpdateId();

        publishEvent(event);
    }
    catch (const std::exception &e)
    {
        Logger::error("Failed to update cache config: " + std::string(e.what()));
    }
}

// Internal methods
void PocoConfigAdapter::publishEvent(const ConfigUpdateEvent &event)
{
    std::lock_guard<std::mutex> lock(observers_mutex_);

    // Log to stdout for immediate visibility
    std::cout << "[CONFIG CHANGE DETECTED] " << event.source << " updated: ";
    for (const auto &key : event.changed_keys)
    {
        std::cout << key << " ";
    }
    std::cout << std::endl;

    Logger::info("Publishing config update event from " + event.source + " with " +
                 std::to_string(event.changed_keys.size()) + " changes");

    for (auto observer : observers_)
    {
        try
        {
            observer->onConfigUpdate(event);
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

std::string PocoConfigAdapter::generateUpdateId() const
{
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    return "update_" + std::to_string(millis);
}

void PocoConfigAdapter::persistChanges(const std::string &changed_key)
{
    try
    {
        // Save configuration to config.json
        if (!poco_cfg_.save("config.json"))
        {
            Logger::error("Failed to persist configuration changes to config.json");
            return;
        }

        Logger::info("Configuration changes persisted to config.json" +
                     (changed_key.empty() ? "" : " (key: " + changed_key + ")"));
    }
    catch (const std::exception &e)
    {
        Logger::error("Exception while persisting configuration: " + std::string(e.what()));
    }
}

// Configuration setters with event publishing
