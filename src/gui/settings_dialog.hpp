#pragma once

#include <SDL2/SDL.h>
#include <string>

namespace mirrolink {
namespace gui {

class SettingsDialog {
public:
    SettingsDialog();
    ~SettingsDialog();

    bool show();
    void hide();
    
    // Settings getters
    int getMaxFps() const { return maxFps; }
    int getBitrate() const { return bitrate; }
    const std::string& getResolution() const { return resolution; }
    bool getEnableAudio() const { return enableAudio; }
    
private:
    void initUI();
    void handleEvents(SDL_Event& event);
    void saveSettings();
    void loadSettings();

    SDL_Window* window;
    SDL_Renderer* renderer;
    bool isVisible;
    
    // Settings
    int maxFps;
    int bitrate;
    std::string resolution;
    bool enableAudio;
};

} // namespace gui
} // namespace mirrolink