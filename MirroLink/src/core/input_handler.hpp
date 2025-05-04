#pragma once

#include <memory>
#include <vector>
#include <cstdint>

namespace mirrolink {

struct TouchEvent {
    uint32_t id;
    float x;
    float y;
    bool pressed;
};

struct KeyboardEvent {
    uint32_t keycode;
    bool pressed;
    bool ctrl;
    bool alt;
    bool shift;
};

struct GamepadEvent {
    uint32_t button;
    float value;
    bool pressed;
};

class InputHandler {
public:
    InputHandler();
    ~InputHandler();

    // Touch input handling
    void sendTouchEvent(const TouchEvent& event);
    void sendMultiTouchEvents(const std::vector<TouchEvent>& events);
    
    // Keyboard input
    void sendKeyEvent(const KeyboardEvent& event);
    void sendText(const std::string& text);
    
    // Special keys
    void sendHome();
    void sendBack();
    void sendAppSwitch();
    
    // Volume controls
    void sendVolumeUp();
    void sendVolumeDown();
    void sendVolumeMute();
    
    // Power management
    void sendPower();
    void sendWake();
    
    // Gamepad support
    void sendGamepadEvent(const GamepadEvent& event);
    bool isGamepadConnected() const;
    
    // Clipboard operations
    bool sendClipboardText(const std::string& text);
    std::string getDeviceClipboardText() const;
    
    // Input mapping
    void setInputMapping(const std::string& mappingFile);
    bool saveInputMapping(const std::string& mappingFile) const;

private:
    class Impl;
    std::unique_ptr<Impl> pimpl;
};