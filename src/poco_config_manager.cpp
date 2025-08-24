#include "core/poco_config_manager.hpp"
#include <fstream>
#include <sstream>

using Poco::AutoPtr;
using Poco::Util::JSONConfiguration;

PocoConfigManager::PocoConfigManager()
{
    cfg_ = new JSONConfiguration();
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
            else
                cfg_->setString(prefix, node.dump());
        }
    };
    apply("", patch);
}

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
