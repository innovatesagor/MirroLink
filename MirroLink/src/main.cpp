#include "gui/main_window.hpp"
#include "utils/logger.hpp"
#include "utils/config_manager.hpp"
#include <iostream>
#include <string>

using namespace mirrolink;

int main(int argc, char* argv[]) {
    // Initialize logging
    utils::Logger::getInstance().setLogFile(utils::ConfigManager::getDefaultLogPath());
    utils::Logger::getInstance().setLogLevel(utils::LogLevel::INFO);
    
    // Load configuration
    auto& config = utils::ConfigManager::getInstance();
    if (!config.loadConfig(utils::ConfigManager::getDefaultConfigPath())) {
        utils::Logger::getInstance().warn("Failed to load config, using defaults");
    }
    
    // Create and initialize main window
    gui::MainWindow window;
    
    int width = config.get<int>("window.width", 1280);
    int height = config.get<int>("window.height", 720);
    std::string title = config.get<std::string>("window.title", "MirroLink");
    
    if (!window.initialize(title.c_str(), width, height)) {
        utils::Logger::getInstance().error("Failed to initialize application window");
        return 1;
    }
    
    // Run the application
    try {
        window.run();
    } catch (const utils::Error& e) {
        utils::Logger::getInstance().fatal("Application error: ", e.what());
        return 1;
    } catch (const std::exception& e) {
        utils::Logger::getInstance().fatal("Unhandled exception: ", e.what());
        return 1;
    }
    
    // Save configuration before exit
    if (!config.saveConfig(utils::ConfigManager::getDefaultConfigPath())) {
        utils::Logger::getInstance().warn("Failed to save configuration");
    }
    
    return 0;
}