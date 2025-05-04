#pragma once

#include <SDL2/SDL.h>
#include "../core/screen_mirror.hpp"
#include "../core/device_manager.hpp"
#include <memory>

namespace mirrolink {
namespace gui {

class DeviceView {
public:
    DeviceView(SDL_Renderer* renderer);
    ~DeviceView();

    // Initialize view
    bool initialize(int width, int height);
    
    // Render current frame
    void render();
    
    // Handle input events
    void handleMouseEvent(const SDL_MouseButtonEvent& event);
    void handleMouseMotion(const SDL_MouseMotionEvent& event);
    void handleKeyEvent(const SDL_KeyboardEvent& event);
    
    // Update frame data
    void updateFrame(const FrameData& frame);
    
    // View properties
    void resize(int width, int height);
    void setAspectRatioMode(bool maintain);
    void setScaleMode(SDL_ScaleMode mode);
    
    // Get current dimensions
    int getWidth() const { return viewWidth; }
    int getHeight() const { return viewHeight; }

private:
    void updateViewport();
    void scaleCoordinates(float& x, float& y);

    SDL_Renderer* renderer;
    SDL_Texture* frameTexture;
    SDL_Rect viewport;
    
    int viewWidth;
    int viewHeight;
    int contentWidth;
    int contentHeight;
    
    bool maintainAspectRatio;
    SDL_ScaleMode scaleMode;
};