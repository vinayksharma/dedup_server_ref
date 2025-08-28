#pragma once

#include "poco_config_manager.hpp"
#include "core/dedup_modes.hpp"
#include "config_observer.hpp"
#include <nlohmann/json.hpp>
#include <memory>
#include <mutex>
#include <vector>
#include <thread>
#include <atomic>
#include <filesystem>

// Forward declarations
class ConfigObserver;

/**
 * @brief Configuration adapter that provides the legacy ServerConfigManager interface
 * but delegates to PocoConfigManager internally.
 *
 * This provides backward compatibility while using the new Poco-based
 * configuration system underneath.
 */
class PocoConfigAdapter
{
public:
    // Singleton pattern - replaces the legacy ServerConfigManager::getInstance()
    static PocoConfigAdapter &getInstance()
    {
        static PocoConfigAdapter instance;
        return instance;
    }

    // Destructor
    ~PocoConfigAdapter();

    // Configuration getters - delegate to PocoConfigManager
    nlohmann::json getAll() const;
    DedupMode getDedupMode() const;
    std::string getLogLevel() const;
    int getServerPort() const;
    std::string getServerHost() const;
    std::string getAuthSecret() const;
    int getScanIntervalSeconds() const;
    int getProcessingIntervalSeconds() const;

    // Thread configuration getters
    int getMaxProcessingThreads() const;
    int getMaxScanThreads() const;

    int getDatabaseThreads() const;

    // Processing configuration getters
    int getProcessingBatchSize() const;

    // File type configuration getters
    std::map<std::string, bool> getSupportedFileTypes() const;
    std::map<std::string, bool> getTranscodingFileTypes() const;

    // File type utility methods
    std::vector<std::string> getEnabledFileTypes() const;
    bool needsTranscoding(const std::string &file_extension) const;

    // Category-specific enabled extensions
    std::vector<std::string> getEnabledImageExtensions() const;
    std::vector<std::string> getEnabledVideoExtensions() const;
    std::vector<std::string> getEnabledAudioExtensions() const;

    // Cache configuration getters
    uint32_t getDecoderCacheSizeMB() const;

    // Cache configuration methods
    std::string getCacheConfig() const;
    bool validateCacheConfig() const;
    void updateCacheConfig(const std::string &json_config);

    // Decoder configuration getters
    int getMaxDecoderThreads() const;

    // Database configuration getters
    int getDatabaseMaxRetries() const;
    int getDatabaseBackoffBaseMs() const;
    int getDatabaseMaxBackoffMs() const;
    int getDatabaseBusyTimeoutMs() const;
    int getDatabaseOperationTimeoutMs() const;

    // Quality stack configuration
    bool getPreProcessQualityStack() const;

    // Video processing configuration accessors
    int getVideoSkipDurationSeconds(DedupMode mode) const;
    int getVideoFramesPerSkip(DedupMode mode) const;
    int getVideoSkipCount(DedupMode mode) const;

    // Enhanced configuration getters for specific categories
    std::string getServerConfig() const;
    std::string getThreadingConfig() const;
    std::string getDatabaseConfig() const;
    std::string getFileTypesConfig() const;
    std::string getVideoConfig() const;
    std::string getScanningConfig() const;
    std::string getLoggingConfig() const;

    // Configuration setters with event publishing
    void setDedupMode(DedupMode mode);
    void setLogLevel(const std::string &level);
    void setServerPort(int port);
    void setAuthSecret(const std::string &secret);
    void setServerHost(const std::string &host);
    void setScanIntervalSeconds(int seconds);
    void setProcessingIntervalSeconds(int seconds);
    void setMaxProcessingThreads(int threads);
    void setMaxScanThreads(int threads);

    void setDatabaseThreads(int threads);
    void setMaxDecoderThreads(int threads);
    void setProcessingBatchSize(int batch_size);
    void setPreProcessQualityStack(bool enabled);
    void setDecoderCacheSizeMB(uint32_t size_mb);
    void setDatabaseMaxRetries(int max_retries);
    void setDatabaseBackoffBaseMs(int backoff_ms);
    void setDatabaseMaxBackoffMs(int max_backoff_ms);
    void setDatabaseBusyTimeoutMs(int timeout_ms);
    void setDatabaseOperationTimeoutMs(int timeout_ms);

    // File type configuration methods
    void setFileTypeEnabled(const std::string &category, const std::string &extension, bool enabled);
    void setTranscodingFileType(const std::string &extension, bool enabled);
    void updateFileTypeConfig(const std::string &category, const std::string &extension, bool enabled);

    // Video processing configuration methods
    void setVideoSkipDurationSeconds(int seconds);
    void setVideoFramesPerSkip(int frames);
    void setVideoSkipCount(int count);

    // Enhanced configuration setters for specific categories
    void updateServerConfig(const std::string &json_config);
    void updateThreadingConfig(const std::string &json_config);
    void updateDatabaseConfig(const std::string &json_config);
    void updateFileTypesConfig(const std::string &json_config);
    void updateVideoConfig(const std::string &json_config);
    void updateScanningConfig(const std::string &json_config);
    void updateLoggingConfig(const std::string &json_config);

    // Remove bulk update method - keeping only specific setters
    // void updateConfigAndPersist(const nlohmann::json &config_updates, const std::vector<std::string> &changed_keys = {});

    void updateConfig(const std::string &json_config);

    // Processing configuration methods
    std::string getProcessingConfig() const;
    void updateProcessingConfig(const std::string &json_config);
    bool validateProcessingConfig() const;

    // Configuration file operations
    bool saveConfig(const std::string &file_path) const;
    bool loadConfig(const std::string &file_path);

    // Configuration validation
    bool validateConfig() const;

    // Runtime config file watching
    void startWatching(const std::string &file_path = "config.json", int interval_seconds = 2);
    void stopWatching();

    // Observer management
    void subscribe(ConfigObserver *observer);
    void unsubscribe(ConfigObserver *observer);

    // Observer management aliases for consistency
    void addObserver(ConfigObserver *observer) { subscribe(observer); }
    void removeObserver(ConfigObserver *observer) { unsubscribe(observer); }

private:
    PocoConfigAdapter();
    PocoConfigAdapter(const PocoConfigAdapter &) = delete;
    PocoConfigAdapter &operator=(const PocoConfigAdapter &) = delete;

    // Internal methods
    void publishEvent(const ConfigUpdateEvent &event);
    void initializeDefaultConfig();
    std::string generateUpdateId() const;
    void persistChanges(const std::string &changed_key = "");

    // Reference to the underlying Poco configuration manager
    PocoConfigManager &poco_cfg_;

    // Observers
    mutable std::mutex observers_mutex_;
    std::vector<ConfigObserver *> observers_;

    // File watching internals
    std::atomic<bool> watching_{false};
    std::thread watcher_thread_;
    std::string watched_file_path_;
    int watch_interval_seconds_{2};
    std::filesystem::file_time_type last_write_time_{};
};
