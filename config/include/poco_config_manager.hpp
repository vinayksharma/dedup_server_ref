#pragma once

#include <Poco/Util/JSONConfiguration.h>
#include <Poco/AutoPtr.h>
#include <mutex>
#include <string>
#include <vector>
#include <map>
#include <nlohmann/json.hpp>
#include "core/dedup_modes.hpp"

class PocoConfigManager
{
public:
    static PocoConfigManager &getInstance()
    {
        static PocoConfigManager instance;
        return instance;
    }

    // Core file operations
    bool load(const std::string &path);
    bool save(const std::string &path) const;
    void update(const nlohmann::json &patch);
    nlohmann::json getAll() const;

    // Basic configuration getters
    std::string getString(const std::string &key, const std::string &def = "") const;
    int getInt(const std::string &key, int def = 0) const;
    bool getBool(const std::string &key, bool def = false) const;
    uint32_t getUInt32(const std::string &key, uint32_t def = 0) const;

    // Server configuration getters
    DedupMode getDedupMode() const;
    std::string getLogLevel() const;
    int getServerPort() const;
    std::string getServerHost() const;
    std::string getAuthSecret() const;

    // Interval configuration getters
    int getScanIntervalSeconds() const;
    int getProcessingIntervalSeconds() const;

    // Thread configuration getters
    int getMaxProcessingThreads() const;
    int getMaxScanThreads() const;

    int getDatabaseThreads() const;
    int getMaxDecoderThreads() const;

    // Processing configuration getters
    int getProcessingBatchSize() const;
    bool getPreProcessQualityStack() const;

    // Database configuration getters
    int getDatabaseMaxRetries() const;
    int getDatabaseBackoffBaseMs() const;
    int getDatabaseMaxBackoffMs() const;
    int getDatabaseBusyTimeoutMs() const;
    int getDatabaseOperationTimeoutMs() const;

    // Cache configuration getters
    uint32_t getDecoderCacheSizeMB() const;

    // File type configuration getters
    std::map<std::string, bool> getSupportedFileTypes() const;
    std::map<std::string, bool> getTranscodingFileTypes() const;
    std::vector<std::string> getEnabledFileTypes() const;
    std::vector<std::string> getEnabledImageExtensions() const;
    std::vector<std::string> getEnabledVideoExtensions() const;
    std::vector<std::string> getEnabledAudioExtensions() const;
    bool needsTranscoding(const std::string &file_extension) const;

    // Video processing configuration getters
    int getVideoSkipDurationSeconds(DedupMode mode) const;
    int getVideoFramesPerSkip(DedupMode mode) const;
    int getVideoSkipCount(DedupMode mode) const;

    // Configuration validation
    bool validateConfig() const;
    bool validateProcessingConfig() const;
    bool validateCacheConfig() const;

    // Configuration sections
    nlohmann::json getProcessingConfig() const;
    nlohmann::json getCacheConfig() const;

    // Utility methods
    void initializeDefaultConfig();
    bool hasKey(const std::string &key) const;
    std::vector<std::string> getKeys(const std::string &prefix = "") const;

private:
    PocoConfigManager();
    ~PocoConfigManager() = default;
    PocoConfigManager(const PocoConfigManager &) = delete;
    PocoConfigManager &operator=(const PocoConfigManager &) = delete;

    // Helper methods for nested configuration
    nlohmann::json getNestedConfig(const std::string &prefix) const;
    std::vector<std::string> getEnabledExtensionsForCategory(const std::string &category) const;

    mutable std::mutex mutex_;
    Poco::AutoPtr<Poco::Util::JSONConfiguration> cfg_;
};

// Helper function to split strings by delimiter
std::vector<std::string> split(const std::string &str, char delimiter);
