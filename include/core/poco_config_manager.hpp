#pragma once

#include <Poco/Util/JSONConfiguration.h>
#include <Poco/AutoPtr.h>
#include <mutex>
#include <string>
#include <nlohmann/json.hpp>

class PocoConfigManager
{
public:
    static PocoConfigManager &getInstance()
    {
        static PocoConfigManager instance;
        return instance;
    }

    bool load(const std::string &path);
    bool save(const std::string &path) const;

    nlohmann::json getAll() const;
    void update(const nlohmann::json &patch);

    // Convenience getters
    std::string getString(const std::string &key, const std::string &def) const;
    int getInt(const std::string &key, int def) const;
    bool getBool(const std::string &key, bool def) const;

private:
    PocoConfigManager();
    mutable std::mutex mutex_;
    Poco::AutoPtr<Poco::Util::JSONConfiguration> cfg_;
};
