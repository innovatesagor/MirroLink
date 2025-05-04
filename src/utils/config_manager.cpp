#include "config_manager.hpp"
#include "logger.hpp"
#include <json/json.h>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

namespace mirrolink {
namespace utils {

class ConfigManager::Impl {
public:
    bool loadConfig(const std::string& path) {
        try {
            std::ifstream file(path);
            if (!file.is_open()) {
                Logger::getInstance().warn("Could not open config file: ", path);
                return false;
            }

            Json::Value root;
            file >> root;

            for (const auto& key : root.getMemberNames()) {
                const auto& value = root[key];
                switch (value.type()) {
                    case Json::ValueType::booleanValue:
                        settings[key] = value.asBool();
                        break;
                    case Json::ValueType::intValue:
                        settings[key] = value.asInt();
                        break;
                    case Json::ValueType::realValue:
                        settings[key] = value.asDouble();
                        break;
                    case Json::ValueType::stringValue:
                        settings[key] = value.asString();
                        break;
                    default:
                        Logger::getInstance().warn("Unsupported value type for key: ", key);
                }
            }

            return true;
        } catch (const std::exception& e) {
            Logger::getInstance().error("Failed to load config: ", e.what());
            return false;
        }
    }

    bool saveConfig(const std::string& path) {
        try {
            // Create directory if it doesn't exist
            fs::path configPath(path);
            fs::create_directories(configPath.parent_path());

            Json::Value root;
            for (const auto& [key, value] : settings) {
                std::visit([&root, &key](const auto& v) {
                    root[key] = v;
                }, value);
            }

            std::ofstream file(path);
            if (!file.is_open()) {
                Logger::getInstance().error("Could not open config file for writing: ", path);
                return false;
            }

            Json::StreamWriterBuilder builder;
            builder["indentation"] = "    ";
            std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
            writer->write(root, &file);

            return true;
        } catch (const std::exception& e) {
            Logger::getInstance().error("Failed to save config: ", e.what());
            return false;
        }
    }

    void remove(const std::string& key) {
        settings.erase(key);
    }

    void clear() {
        settings.clear();
    }

    bool hasKey(const std::string& key) const {
        return settings.find(key) != settings.end();
    }

    std::unordered_map<std::string, ConfigValue>& getSettings() {
        return settings;
    }

private:
    std::unordered_map<std::string, ConfigValue> settings;
};

// Static instance
ConfigManager& ConfigManager::getInstance() {
    static ConfigManager instance;
    return instance;
}

// Constructor/Destructor
ConfigManager::ConfigManager() : pimpl(std::make_unique<Impl>()) {}
ConfigManager::~ConfigManager() = default;

// Public interface implementation
bool ConfigManager::loadConfig(const std::string& path) {
    return pimpl->loadConfig(path);
}

bool ConfigManager::saveConfig(const std::string& path) {
    return pimpl->saveConfig(path);
}

void ConfigManager::remove(const std::string& key) {
    pimpl->remove(key);
}

void ConfigManager::clear() {
    pimpl->clear();
}

bool ConfigManager::hasKey(const std::string& key) const {
    return pimpl->hasKey(key);
}

std::string ConfigManager::getDefaultConfigPath() {
    const char* homeDir = std::getenv("HOME");
    if (!homeDir) {
        return "/tmp/mirrolink.json";
    }
    return std::string(homeDir) + "/.config/mirrolink/config.json";
}

std::string ConfigManager::getDefaultLogPath() {
    const char* homeDir = std::getenv("HOME");
    if (!homeDir) {
        return "/tmp/mirrolink.log";
    }
    return std::string(homeDir) + "/.local/share/mirrolink/mirrolink.log";
}

}} // namespace mirrolink::utils