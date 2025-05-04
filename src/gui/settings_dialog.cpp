#include "settings_dialog.hpp"
#include "../utils/config_manager.hpp"
#include "../utils/logger.hpp"

namespace mirrolink {
namespace gui {

SettingsDialog::SettingsDialog()
    : window(nullptr)
    , renderer(nullptr)
    , isVisible(false)
    , maxFps(60)
    , bitrate(8000000)
    , resolution("1920x1080")
    , enableAudio(true) {
    loadSettings();
    initUI();
}

SettingsDialog::~SettingsDialog() {
    if (renderer) {
        SDL_DestroyRenderer(renderer);
    }
    if (window) {
        SDL_DestroyWindow(window);
    }
}

bool SettingsDialog::show() {
    if (!window) {
        utils::Logger::getInstance().error("Settings dialog window not initialized");
        return false;
    }
    
    isVisible = true;
    SDL_ShowWindow(window);
    return true;
}

void SettingsDialog::hide() {
    if (window) {
        SDL_HideWindow(window);
        isVisible = false;
    }
}

void SettingsDialog::initUI() {
    window = SDL_CreateWindow(
        "MirroLink Settings",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        400,
        300,
        SDL_WINDOW_HIDDEN | SDL_WINDOW_ALLOW_HIGHDPI
    );
    
    if (!window) {
        utils::Logger::getInstance().error("Failed to create settings window: ", SDL_GetError());
        return;
    }
    
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        utils::Logger::getInstance().error("Failed to create settings renderer: ", SDL_GetError());
        return;
    }
}

void SettingsDialog::handleEvents(SDL_Event& event) {
    // TODO: Implement settings UI event handling
}

void SettingsDialog::saveSettings() {
    auto& config = utils::ConfigManager::getInstance();
    config.set("display.maxFps", maxFps);
    config.set("display.bitrate", bitrate);
    config.set("display.resolution", resolution);
    config.set("audio.enabled", enableAudio);
    config.saveConfig(utils::ConfigManager::getDefaultConfigPath());
}

void SettingsDialog::loadSettings() {
    auto& config = utils::ConfigManager::getInstance();
    maxFps = config.get<int>("display.maxFps", 60);
    bitrate = config.get<int>("display.bitrate", 8000000);
    resolution = config.get<std::string>("display.resolution", "1920x1080");
    enableAudio = config.get<bool>("audio.enabled", true);
}

} // namespace gui
} // namespace mirrolink