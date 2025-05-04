#pragma once

#include <SDL2/SDL.h>
#include "../core/device_manager.hpp"
#include "../core/screen_mirror.hpp"
#include <memory>

namespace mirrolink {
namespace gui {

class MainWindow {
public:
    MainWindow();
    ~MainWindow();

    // Initialize window and SDL systems
    bool initialize(const char* title, int width, int height);
    
    // Main event loop
    void run();
    
    // Window management
    void setFullscreen(bool fullscreen);
    bool isFullscreen() const;
    void resize(int width, int height);
    
    // Device connection handling
    void onDeviceConnected(const DeviceInfo& device);
    void onDeviceDisconnected(const DeviceInfo& device);
    
    // Frame handling
    void onFrameReceived(const FrameData& frame);

private:
    // Event handlers
    void handleKeyboard(const SDL_KeyboardEvent& event);
    void handleMouse(const SDL_MouseButtonEvent& event);
    void handleMouseMotion(const SDL_MouseMotionEvent& event);
    
    // Window state
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* frameTexture;
    
    // Core components
    std::unique_ptr<DeviceManager> deviceManager;
    std::unique_ptr<ScreenMirror> screenMirror;
    
    // Window properties
    int windowWidth;
    int windowHeight;
    bool isRunning;
    bool fullscreenMode;
};

}} // namespace mirrolink::gui