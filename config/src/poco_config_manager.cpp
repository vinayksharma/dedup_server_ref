#include "poco_config_manager.hpp"
#include "logging/logger.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

using Poco::AutoPtr;
using Poco::Util::JSONConfiguration;

PocoConfigManager::PocoConfigManager()
{
    cfg_ = new JSONConfiguration();
    initializeDefaultConfig();
}

bool PocoConfigManager::load(const std::string &path)
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::ifstream in(path);
    if (!in.good())
        return false;
    AutoPtr<JSONConfiguration> tmp = new JSONConfiguration();
    tmp->load(in);
    cfg_ = tmp;
    return true;
}

bool PocoConfigManager::save(const std::string &path) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::ofstream out(path);
    if (!out.is_open())
        return false;
    cfg_->save(out);
    return true;
}

nlohmann::json PocoConfigManager::getAll() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::stringstream ss;
    cfg_->save(ss);
    return nlohmann::json::parse(ss.str());
}

void PocoConfigManager::update(const nlohmann::json &patch)
{
    std::lock_guard<std::mutex> lock(mutex_);
    // Flatten and set values
    std::function<void(const std::string &, const nlohmann::json &)> apply;
    apply = [&](const std::string &prefix, const nlohmann::json &node)
    {
        if (node.is_object())
        {
            for (auto it = node.begin(); it != node.end(); ++it)
            {
                std::string key = prefix.empty() ? it.key() : (prefix + "." + it.key());
                apply(key, it.value());
            }
        }
        else if (!node.is_null())
        {
            if (node.is_boolean())
                cfg_->setBool(prefix, node.get<bool>());
            else if (node.is_number_integer())
                cfg_->setInt(prefix, node.get<int>());
            else if (node.is_number_unsigned())
                cfg_->setUInt(prefix, static_cast<unsigned>(node.get<unsigned long long>()));
            else if (node.is_number_float())
                cfg_->setDouble(prefix, node.get<double>());
            else if (node.is_string())
                cfg_->setString(prefix, node.get<std::string>());
            else
                cfg_->setString(prefix, node.dump());
        }
    };
    apply("", patch);
}

// Basic configuration getters
std::string PocoConfigManager::getString(const std::string &key, const std::string &def) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return cfg_->getString(key, def);
}

int PocoConfigManager::getInt(const std::string &key, int def) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return cfg_->getInt(key, def);
}

bool PocoConfigManager::getBool(const std::string &key, bool def) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return cfg_->getBool(key, def);
}

uint32_t PocoConfigManager::getUInt32(const std::string &key, uint32_t def) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return static_cast<uint32_t>(cfg_->getUInt(key, def));
}

// Server configuration getters
DedupMode PocoConfigManager::getDedupMode() const
{
    std::string mode_str = getString("dedup_mode", "BALANCED");
    return DedupModes::fromString(mode_str);
}

std::string PocoConfigManager::getLogLevel() const
{
    return getString("log_level", "INFO");
}

int PocoConfigManager::getServerPort() const
{
    return getInt("server_port", 8080);
}

std::string PocoConfigManager::getServerHost() const
{
    return getString("server_host", "localhost");
}

std::string PocoConfigManager::getAuthSecret() const
{
    return getString("auth_secret", "your-secret-key-here");
}

// Interval configuration getters
int PocoConfigManager::getScanIntervalSeconds() const
{
    return getInt("scan_interval_seconds", 3600);
}

int PocoConfigManager::getProcessingIntervalSeconds() const
{
    return getInt("processing_interval_seconds", 1800);
}

// Thread configuration getters
int PocoConfigManager::getMaxProcessingThreads() const
{
    return getInt("threading.max_processing_threads", 8);
}

int PocoConfigManager::getMaxScanThreads() const
{
    return getInt("threading.max_scan_threads", 4);
}

std::string PocoConfigManager::getHttpServerThreads() const
{
    return getString("threading.http_server_threads", "auto");
}

int PocoConfigManager::getDatabaseThreads() const
{
    return getInt("threading.database_threads", 2);
}

int PocoConfigManager::getMaxDecoderThreads() const
{
    return getInt("threading.max_decoder_threads", 4);
}

// Processing configuration getters
int PocoConfigManager::getProcessingBatchSize() const
{
    return getInt("processing.batch_size", 100);
}

bool PocoConfigManager::getPreProcessQualityStack() const
{
    return getBool("pre_process_quality_stack", false);
}

// Database configuration getters
int PocoConfigManager::getDatabaseMaxRetries() const
{
    return getInt("database.retry.max_attempts", 3);
}

int PocoConfigManager::getDatabaseBackoffBaseMs() const
{
    return getInt("database.retry.backoff_base_ms", 100);
}

int PocoConfigManager::getDatabaseMaxBackoffMs() const
{
    return getInt("database.retry.max_backoff_ms", 1000);
}

int PocoConfigManager::getDatabaseBusyTimeoutMs() const
{
    return getInt("database.timeout.busy_timeout_ms", 30000);
}

int PocoConfigManager::getDatabaseOperationTimeoutMs() const
{
    return getInt("database.timeout.operation_timeout_ms", 60000);
}

// Cache configuration getters
uint32_t PocoConfigManager::getDecoderCacheSizeMB() const
{
    return getUInt32("cache.decoder_cache_size_mb", 1024);
}

// File type configuration getters
std::map<std::string, bool> PocoConfigManager::getSupportedFileTypes() const
{
    std::map<std::string, bool> supported_types;

    // Get all categories
    auto categories = getNestedConfig("categories");
    for (auto category_it = categories.begin(); category_it != categories.end(); ++category_it)
    {
        if (category_it.value().is_object())
        {
            for (auto ext_it = category_it.value().begin(); ext_it != category_it.value().end(); ++ext_it)
            {
                if (ext_it.value().is_boolean())
                {
                    supported_types[ext_it.key()] = ext_it.value().get<bool>();
                }
            }
        }
    }

    return supported_types;
}

std::map<std::string, bool> PocoConfigManager::getTranscodingFileTypes() const
{
    std::map<std::string, bool> transcoding_types;

    // Get video and audio types that need transcoding
    auto video_types = getNestedConfig("categories.video");
    auto audio_types = getNestedConfig("categories.audio");

    for (auto it = video_types.begin(); it != video_types.end(); ++it)
    {
        if (it.value().is_boolean())
        {
            transcoding_types[it.key()] = it.value().get<bool>();
        }
    }

    for (auto it = audio_types.begin(); it != audio_types.end(); ++it)
    {
        if (it.value().is_boolean())
        {
            transcoding_types[it.key()] = it.value().get<bool>();
        }
    }

    return transcoding_types;
}

std::vector<std::string> PocoConfigManager::getEnabledFileTypes() const
{
    std::vector<std::string> enabled_types;
    auto supported = getSupportedFileTypes();

    for (const auto &pair : supported)
    {
        if (pair.second)
        {
            enabled_types.push_back(pair.first);
        }
    }

    return enabled_types;
}

std::vector<std::string> PocoConfigManager::getEnabledImageExtensions() const
{
    return getEnabledExtensionsForCategory("images");
}

std::vector<std::string> PocoConfigManager::getEnabledVideoExtensions() const
{
    return getEnabledExtensionsForCategory("video");
}

std::vector<std::string> PocoConfigManager::getEnabledAudioExtensions() const
{
    return getEnabledExtensionsForCategory("audio");
}

bool PocoConfigManager::needsTranscoding(const std::string &file_extension) const
{
    std::string ext = file_extension;
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    auto transcoding_types = getTranscodingFileTypes();
    return transcoding_types.find(ext) != transcoding_types.end() && transcoding_types[ext];
}

// Video processing configuration getters
int PocoConfigManager::getVideoSkipDurationSeconds(DedupMode mode) const
{
    std::string mode_str = DedupModes::getModeName(mode);
    return getInt("video_processing." + mode_str + ".skip_duration_seconds", 1);
}

int PocoConfigManager::getVideoFramesPerSkip(DedupMode mode) const
{
    std::string mode_str = DedupModes::getModeName(mode);
    return getInt("video_processing." + mode_str + ".frames_per_skip", 2);
}

int PocoConfigManager::getVideoSkipCount(DedupMode mode) const
{
    std::string mode_str = DedupModes::getModeName(mode);
    return getInt("video_processing." + mode_str + ".skip_count", 8);
}

// Configuration validation
bool PocoConfigManager::validateConfig() const
{
    std::vector<std::string> required_fields = {
        "dedup_mode", "log_level", "server_port", "server_host", "auth_secret"};

    for (const auto &field : required_fields)
    {
        if (!hasKey(field))
        {
            Logger::error("Missing required config field: " + field);
            return false;
        }
    }

    int port = getServerPort();
    if (port <= 0 || port > 65535)
    {
        Logger::error("Invalid server port: " + std::to_string(port));
        return false;
    }

    std::string mode = getString("dedup_mode");
    if (mode != "FAST" && mode != "BALANCED" && mode != "QUALITY")
    {
        Logger::error("Invalid dedup mode: " + mode);
        return false;
    }

    std::string log_level = getLogLevel();
    if (log_level != "TRACE" && log_level != "DEBUG" && log_level != "INFO" &&
        log_level != "WARN" && log_level != "ERROR")
    {
        Logger::error("Invalid log level: " + log_level);
        return false;
    }

    return true;
}

bool PocoConfigManager::validateProcessingConfig() const
{
    // Basic validation for processing config
    int batch_size = getProcessingBatchSize();
    if (batch_size <= 0 || batch_size > 10000)
    {
        Logger::error("Invalid processing batch size: " + std::to_string(batch_size));
        return false;
    }

    return true;
}

bool PocoConfigManager::validateCacheConfig() const
{
    // Basic validation for cache config
    uint32_t cache_size = getDecoderCacheSizeMB();
    if (cache_size == 0 || cache_size > 100000) // Max 100GB
    {
        Logger::error("Invalid decoder cache size: " + std::to_string(cache_size));
        return false;
    }

    return true;
}

// Configuration sections
nlohmann::json PocoConfigManager::getProcessingConfig() const
{
    nlohmann::json processing_config;
    processing_config["max_processing_threads"] = getMaxProcessingThreads();
    processing_config["max_scan_threads"] = getMaxScanThreads();
    processing_config["max_decoder_threads"] = getMaxDecoderThreads();
    processing_config["batch_size"] = getProcessingBatchSize();
    processing_config["dedup_mode"] = getString("dedup_mode");
    processing_config["pre_process_quality_stack"] = getPreProcessQualityStack();
    return processing_config;
}

nlohmann::json PocoConfigManager::getCacheConfig() const
{
    nlohmann::json cache_config;
    cache_config["decoder_cache_size_mb"] = getDecoderCacheSizeMB();

    // Add cache cleanup settings
    cache_config["cache_cleanup"] = {
        {"fully_processed_age_days", getInt("cache_cleanup.fully_processed_age_days", 7)},
        {"partially_processed_age_days", getInt("cache_cleanup.partially_processed_age_days", 3)},
        {"unprocessed_age_days", getInt("cache_cleanup.unprocessed_age_days", 1)},
        {"require_all_modes", getBool("cache_cleanup.require_all_modes", true)},
        {"cleanup_threshold_percent", getInt("cache_cleanup.cleanup_threshold_percent", 80)}};

    return cache_config;
}

// Utility methods
void PocoConfigManager::initializeDefaultConfig()
{
    std::lock_guard<std::mutex> lock(mutex_);

    // Set default configuration values
    cfg_->setString("auth_secret", "your-secret-key-here");
    cfg_->setString("dedup_mode", "BALANCED");
    cfg_->setString("log_level", "INFO");
    cfg_->setInt("server_port", 8080);
    cfg_->setString("server_host", "localhost");
    cfg_->setInt("scan_interval_seconds", 3600);
    cfg_->setInt("processing_interval_seconds", 1800);
    cfg_->setBool("pre_process_quality_stack", false);

    // Threading defaults
    cfg_->setInt("threading.max_processing_threads", 8);
    cfg_->setInt("threading.max_scan_threads", 4);
    cfg_->setString("threading.http_server_threads", "auto");
    cfg_->setInt("threading.database_threads", 2);
    cfg_->setInt("threading.max_decoder_threads", 4);

    // Database defaults
    cfg_->setInt("database.retry.max_attempts", 3);
    cfg_->setInt("database.retry.backoff_base_ms", 100);
    cfg_->setInt("database.retry.max_backoff_ms", 1000);
    cfg_->setInt("database.timeout.busy_timeout_ms", 30000);
    cfg_->setInt("database.timeout.operation_timeout_ms", 60000);

    // Cache defaults
    cfg_->setUInt("cache.decoder_cache_size_mb", 1024);

    // Processing defaults
    cfg_->setInt("processing.batch_size", 100);

    // Cache cleanup defaults
    cfg_->setInt("cache_cleanup.fully_processed_age_days", 7);
    cfg_->setInt("cache_cleanup.partially_processed_age_days", 3);
    cfg_->setInt("cache_cleanup.unprocessed_age_days", 1);
    cfg_->setBool("cache_cleanup.require_all_modes", true);
    cfg_->setInt("cache_cleanup.cleanup_threshold_percent", 80);

    // File type categories
    cfg_->setBool("categories.images.jpg", true);
    cfg_->setBool("categories.images.jpeg", true);
    cfg_->setBool("categories.images.png", true);
    cfg_->setBool("categories.images.bmp", true);
    cfg_->setBool("categories.images.gif", true);
    cfg_->setBool("categories.images.tiff", true);
    cfg_->setBool("categories.images.webp", true);
    cfg_->setBool("categories.images.jp2", true);

    cfg_->setBool("categories.video.mp4", true);
    cfg_->setBool("categories.video.avi", true);
    cfg_->setBool("categories.video.mov", true);
    cfg_->setBool("categories.video.mkv", true);
    cfg_->setBool("categories.video.wmv", true);
    cfg_->setBool("categories.video.flv", true);
    cfg_->setBool("categories.video.webm", true);

    cfg_->setBool("categories.audio.mp3", true);
    cfg_->setBool("categories.audio.wav", true);
    cfg_->setBool("categories.audio.flac", true);
    cfg_->setBool("categories.audio.ogg", true);
    cfg_->setBool("categories.audio.m4a", true);
    cfg_->setBool("categories.audio.aac", true);

    // Video processing defaults
    cfg_->setInt("video_processing.FAST.skip_duration_seconds", 2);
    cfg_->setInt("video_processing.FAST.frames_per_skip", 2);
    cfg_->setInt("video_processing.FAST.skip_count", 5);

    cfg_->setInt("video_processing.BALANCED.skip_duration_seconds", 1);
    cfg_->setInt("video_processing.BALANCED.frames_per_skip", 2);
    cfg_->setInt("video_processing.BALANCED.skip_count", 8);

    cfg_->setInt("video_processing.QUALITY.skip_duration_seconds", 1);
    cfg_->setInt("video_processing.QUALITY.frames_per_skip", 3);
    cfg_->setInt("video_processing.QUALITY.skip_count", 12);
}

bool PocoConfigManager::hasKey(const std::string &key) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    try
    {
        cfg_->getString(key);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

std::vector<std::string> PocoConfigManager::getKeys(const std::string &prefix) const
{
    std::vector<std::string> keys;
    // This is a simplified implementation - Poco doesn't provide direct key enumeration
    // In a real implementation, you might need to parse the JSON structure
    return keys;
}

// Helper methods for nested configuration
nlohmann::json PocoConfigManager::getNestedConfig(const std::string &prefix) const
{
    std::lock_guard<std::mutex> lock(mutex_);

    try
    {
        // Get the full config and extract the nested section
        std::stringstream ss;
        cfg_->save(ss);
        auto full_config = nlohmann::json::parse(ss.str());

        // Navigate to the nested section
        auto keys = split(prefix, '.');
        auto current = full_config;

        for (const auto &key : keys)
        {
            if (current.contains(key) && current[key].is_object())
            {
                current = current[key];
            }
            else
            {
                return nlohmann::json::object();
            }
        }

        return current;
    }
    catch (...)
    {
        return nlohmann::json::object();
    }
}

std::vector<std::string> PocoConfigManager::getEnabledExtensionsForCategory(const std::string &category) const
{
    std::vector<std::string> enabled_extensions;

    try
    {
        auto category_config = getNestedConfig("categories." + category);
        if (category_config.is_object())
        {
            for (auto it = category_config.begin(); it != category_config.end(); ++it)
            {
                if (it.value().is_boolean() && it.value().get<bool>())
                {
                    enabled_extensions.push_back(it.key());
                }
            }
        }
    }
    catch (...)
    {
        // Return empty vector on error
    }

    return enabled_extensions;
}

// Helper function to split strings by delimiter
std::vector<std::string> split(const std::string &str, char delimiter)
{
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;

    while (std::getline(ss, token, delimiter))
    {
        tokens.push_back(token);
    }

    return tokens;
}
