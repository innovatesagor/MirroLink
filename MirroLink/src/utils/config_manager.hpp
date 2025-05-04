#pragma once

#include <memory>
#include <string>
#include <optional>
#include <variant>
#include <unordered_map>

namespace mirrolink {
namespace utils {

using ConfigValue = std::variant<bool, int, double, std::string>;

class ConfigManager {
public:
    static ConfigManager& getInstance();

    // Configuration file operations
    bool loadConfig(const std::string& path);
    bool saveConfig(const std::string& path);
    
    // Get configuration values with default fallback
    template<typename T>
    T get(const std::string& key, const T& defaultValue) const {
        auto it = settings.find(key);
        if (it != settings.end()) {
            try {
                return std::get<T>(it->second);
            } catch (...) {
                return defaultValue;
            }
        }
        return defaultValue;
    }
    
    // Get optional configuration value
    template<typename T>
    std::optional<T> get(const std::string& key) const {
        auto it = settings.find(key);
        if (it != settings.end()) {
            try {
                return std::get<T>(it->second);
            } catch (...) {
                return std::nullopt;
            }
        }
        return std::nullopt;
    }
    
    // Set configuration value
    template<typename T>
    void set(const std::string& key, const T& value) {
        settings[key] = value;
    }
    
    // Remove configuration value
    void remove(const std::string& key);
    
    // Clear all settings
    void clear();
    
    // Check if key exists
    bool hasKey(const std::string& key) const;
    
    // Default paths
    static std::string getDefaultConfigPath();
    static std::string getDefaultLogPath();

private:
    ConfigManager();
    ~ConfigManager();
    
    std::unordered_map<std::string, ConfigValue> settings;
    
    class Impl;
    std::unique_ptr<Impl> pimpl;
    
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;
};