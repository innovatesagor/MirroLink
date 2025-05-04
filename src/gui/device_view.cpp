#include "device_view.hpp"
#include "../utils/logger.hpp"
#include <algorithm>

namespace mirrolink {
namespace gui {

DeviceView::DeviceView(SDL_Renderer* renderer)
    : renderer(renderer)
    , frameTexture(nullptr)
    , viewWidth(0)
    , viewHeight(0)
    , contentWidth(0)
    , contentHeight(0)
    , maintainAspectRatio(true)
    , scaleMode(SDL_ScaleMode::SDL_ScaleModeLinear)
{
    viewport = {0, 0, 0, 0};
}

DeviceView::~DeviceView() {
    if (frameTexture) {
        SDL_DestroyTexture(frameTexture);
    }
}

bool DeviceView::initialize(int width, int height) {
    viewWidth = width;
    viewHeight = height;
    updateViewport();
    return true;
}

void DeviceView::render() {
    if (!frameTexture) return;
    
    SDL_RenderSetViewport(renderer, &viewport);
    SDL_SetTextureScaleMode(frameTexture, scaleMode);
    SDL_RenderCopy(renderer, frameTexture, nullptr, nullptr);
}

void DeviceView::handleMouseEvent(const SDL_MouseButtonEvent& event) {
    if (!frameTexture) return;
    
    float x = static_cast<float>(event.x - viewport.x);
    float y = static_cast<float>(event.y - viewport.y);
    
    scaleCoordinates(x, y);
    
    TouchEvent touchEvent{
        .id = event.which,
        .x = x,
        .y = y,
        .pressed = event.type == SDL_MOUSEBUTTONDOWN
    };
    
    // Forward to input handler through main window
}

void DeviceView::handleMouseMotion(const SDL_MouseMotionEvent& event) {
    if (!frameTexture || !(event.state & SDL_BUTTON_LMASK)) return;
    
    float x = static_cast<float>(event.x - viewport.x);
    float y = static_cast<float>(event.y - viewport.y);
    
    scaleCoordinates(x, y);
    
    TouchEvent touchEvent{
        .id = event.which,
        .x = x,
        .y = y,
        .pressed = true
    };
    
    // Forward to input handler through main window
}

void DeviceView::handleKeyEvent(const SDL_KeyboardEvent& event) {
    if (!frameTexture) return;
    
    KeyboardEvent keyEvent{
        .keycode = event.keysym.scancode,
        .pressed = event.type == SDL_KEYDOWN,
        .ctrl = (event.keysym.mod & KMOD_CTRL) != 0,
        .alt = (event.keysym.mod & KMOD_ALT) != 0,
        .shift = (event.keysym.mod & KMOD_SHIFT) != 0
    };
    
    // Forward to input handler through main window
}

void DeviceView::updateFrame(const FrameData& frame) {
    if (!frameTexture || 
        frame.width != contentWidth || 
        frame.height != contentHeight) {
        
        if (frameTexture) {
            SDL_DestroyTexture(frameTexture);
        }
        
        frameTexture = SDL_CreateTexture(
            renderer,
            SDL_PIXELFORMAT_RGBA8888,
            SDL_TEXTUREACCESS_STREAMING,
            frame.width,
            frame.height
        );
        
        if (!frameTexture) {
            utils::Logger::getInstance().error("Failed to create texture: ", SDL_GetError());
            return;
        }
        
        contentWidth = frame.width;
        contentHeight = frame.height;
        updateViewport();
    }
    
    SDL_UpdateTexture(frameTexture, nullptr, frame.data.data(), frame.width * 4);
}

void DeviceView::resize(int width, int height) {
    viewWidth = width;
    viewHeight = height;
    updateViewport();
}

void DeviceView::setAspectRatioMode(bool maintain) {
    maintainAspectRatio = maintain;
    updateViewport();
}

void DeviceView::setScaleMode(SDL_ScaleMode mode) {
    scaleMode = mode;
}

void DeviceView::updateViewport() {
    if (!contentWidth || !contentHeight || !viewWidth || !viewHeight) {
        viewport = {0, 0, viewWidth, viewHeight};
        return;
    }
    
    if (maintainAspectRatio) {
        float contentRatio = static_cast<float>(contentWidth) / contentHeight;
        float viewRatio = static_cast<float>(viewWidth) / viewHeight;
        
        if (contentRatio > viewRatio) {
            // Fit to width
            viewport.w = viewWidth;
            viewport.h = static_cast<int>(viewWidth / contentRatio);
            viewport.x = 0;
            viewport.y = (viewHeight - viewport.h) / 2;
        } else {
            // Fit to height
            viewport.h = viewHeight;
            viewport.w = static_cast<int>(viewHeight * contentRatio);
            viewport.x = (viewWidth - viewport.w) / 2;
            viewport.y = 0;
        }
    } else {
        viewport = {0, 0, viewWidth, viewHeight};
    }
}

void DeviceView::scaleCoordinates(float& x, float& y) {
    if (!contentWidth || !contentHeight) return;
    
    // Convert viewport coordinates to content coordinates
    x = std::clamp(x / viewport.w, 0.0f, 1.0f);
    y = std::clamp(y / viewport.h, 0.0f, 1.0f);
}

}} // namespace mirrolink::gui