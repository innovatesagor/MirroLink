#include "input_handler.hpp"
#include "../utils/logger.hpp"
#include "../utils/error.hpp"
#include <json/json.h>
#include <fstream>
#include <map>
#include <thread>
#include <array>
#include <sstream>
#include <memory>
#include <cstdio>

namespace mirrolink {

class AdbCommand {
public:
    static std::string execute(const std::string& command, bool checkResult = true) {
        std::array<char, 128> buffer;
        std::string result;
        
        std::string fullCommand = "adb " + command;
        std::unique_ptr<FILE, decltype(&pclose)> pipe(
            popen(fullCommand.c_str(), "r"), pclose);
            
        if (!pipe) {
            throw utils::Error("Failed to execute ADB command: " + command);
        }
        
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            result += buffer.data();
        }
        
        if (checkResult && result.find("error") != std::string::npos) {
            throw utils::Error("ADB command failed: " + result);
        }
        
        return result;
    }
};

class InputHandler::Impl {
public:
    Impl() {
        // Initialize default key mappings
        initializeDefaultMappings();
        
        // Verify ADB is available
        try {
            AdbCommand::execute("version", false);
        } catch (const utils::Error& e) {
            utils::Logger::getInstance().error("ADB not available: ", e.what());
        }
    }
    
    void sendTouchEvent(const TouchEvent& event) {
        try {
            // Convert screen coordinates to Android coordinates
            int x = static_cast<int>(event.x * screenWidth);
            int y = static_cast<int>(event.y * screenHeight);
            
            std::stringstream ss;
            ss << "shell input touchscreen " 
               << (event.pressed ? "down " : "up ")
               << x << " " << y;
               
            AdbCommand::execute(ss.str());
        } catch (const utils::Error& e) {
            utils::Logger::getInstance().error("Failed to send touch event: ", e.what());
        }
    }
    
    void sendKeyEvent(const KeyboardEvent& event) {
        try {
            auto it = keyMap.find(event.keycode);
            if (it != keyMap.end()) {
                std::stringstream ss;
                ss << "shell input keyevent ";
                
                if (event.ctrl) ss << "CTRL ";
                if (event.alt) ss << "ALT ";
                if (event.shift) ss << "SHIFT ";
                
                ss << it->second;
                
                AdbCommand::execute(ss.str());
            }
        } catch (const utils::Error& e) {
            utils::Logger::getInstance().error("Failed to send key event: ", e.what());
        }
    }
    
    void sendText(const std::string& text) {
        try {
            std::string escapedText = escapeString(text);
            AdbCommand::execute("shell input text '" + escapedText + "'");
        } catch (const utils::Error& e) {
            utils::Logger::getInstance().error("Failed to send text: ", e.what());
        }
    }
    
    void sendHome() {
        try {
            AdbCommand::execute("shell input keyevent KEYCODE_HOME");
        } catch (const utils::Error& e) {
            utils::Logger::getInstance().error("Failed to send HOME: ", e.what());
        }
    }
    
    void sendBack() {
        try {
            AdbCommand::execute("shell input keyevent KEYCODE_BACK");
        } catch (const utils::Error& e) {
            utils::Logger::getInstance().error("Failed to send BACK: ", e.what());
        }
    }
    
    void sendAppSwitch() {
        try {
            AdbCommand::execute("shell input keyevent KEYCODE_APP_SWITCH");
        } catch (const utils::Error& e) {
            utils::Logger::getInstance().error("Failed to send APP_SWITCH: ", e.what());
        }
    }
    
    void sendVolumeUp() {
        try {
            AdbCommand::execute("shell input keyevent KEYCODE_VOLUME_UP");
        } catch (const utils::Error& e) {
            utils::Logger::getInstance().error("Failed to send VOLUME_UP: ", e.what());
        }
    }
    
    void sendVolumeDown() {
        try {
            AdbCommand::execute("shell input keyevent KEYCODE_VOLUME_DOWN");
        } catch (const utils::Error& e) {
            utils::Logger::getInstance().error("Failed to send VOLUME_DOWN: ", e.what());
        }
    }
    
    void sendVolumeMute() {
        try {
            AdbCommand::execute("shell input keyevent KEYCODE_VOLUME_MUTE");
        } catch (const utils::Error& e) {
            utils::Logger::getInstance().error("Failed to send VOLUME_MUTE: ", e.what());
        }
    }
    
    void sendPower() {
        try {
            AdbCommand::execute("shell input keyevent KEYCODE_POWER");
        } catch (const utils::Error& e) {
            utils::Logger::getInstance().error("Failed to send POWER: ", e.what());
        }
    }
    
    void sendWake() {
        try {
            AdbCommand::execute("shell input keyevent KEYCODE_WAKEUP");
        } catch (const utils::Error& e) {
            utils::Logger::getInstance().error("Failed to send WAKEUP: ", e.what());
        }
    }
    
    bool sendClipboardText(const std::string& text) {
        try {
            std::string escapedText = escapeString(text);
            AdbCommand::execute("shell am broadcast -a clipper.set -e text '" + escapedText + "'");
            return true;
        } catch (const utils::Error& e) {
            utils::Logger::getInstance().error("Failed to set clipboard: ", e.what());
            return false;
        }
    }
    
    std::string getDeviceClipboardText() const {
        try {
            return AdbCommand::execute("shell am broadcast -a clipper.get");
        } catch (const utils::Error& e) {
            utils::Logger::getInstance().error("Failed to get clipboard: ", e.what());
            return "";
        }
    }
    
    bool setInputMapping(const std::string& mappingFile) {
        try {
            std::ifstream file(mappingFile);
            Json::Value root;
            file >> root;
            
            keyMap.clear();
            for (const auto& member : root.getMemberNames()) {
                keyMap[std::stoi(member)] = root[member].asString();
            }
            
            return true;
        } catch (const std::exception& e) {
            utils::Logger::getInstance().error("Failed to load mapping file: ", e.what());
            return false;
        }
    }
    
    bool saveInputMapping(const std::string& mappingFile) const {
        try {
            Json::Value root;
            for (const auto& pair : keyMap) {
                root[std::to_string(pair.first)] = pair.second;
            }
            
            std::ofstream file(mappingFile);
            file << root;
            return true;
        } catch (const std::exception& e) {
            utils::Logger::getInstance().error("Failed to save mapping file: ", e.what());
            return false;
        }
    }
    
    bool isGamepadConnected() const {
        try {
            std::string result = AdbCommand::execute("shell dumpsys input", false);
            return result.find("Gamepad") != std::string::npos;
        } catch (const utils::Error& e) {
            utils::Logger::getInstance().error("Failed to check gamepad: ", e.what());
            return false;
        }
    }
    
    void sendGamepadEvent(const GamepadEvent& event) {
        try {
            // Map gamepad buttons to Android keycodes
            std::string keycode;
            switch (event.button) {
                case 0: keycode = "KEYCODE_BUTTON_A"; break;     // A button
                case 1: keycode = "KEYCODE_BUTTON_B"; break;     // B button
                case 2: keycode = "KEYCODE_BUTTON_X"; break;     // X button
                case 3: keycode = "KEYCODE_BUTTON_Y"; break;     // Y button
                case 4: keycode = "KEYCODE_BUTTON_L1"; break;    // Left bumper
                case 5: keycode = "KEYCODE_BUTTON_R1"; break;    // Right bumper
                case 6: keycode = "KEYCODE_BUTTON_SELECT"; break;// Select/Back
                case 7: keycode = "KEYCODE_BUTTON_START"; break; // Start
                case 8: keycode = "KEYCODE_BUTTON_THUMBL"; break;// Left stick press
                case 9: keycode = "KEYCODE_BUTTON_THUMBR"; break;// Right stick press
                case 10: keycode = "KEYCODE_DPAD_UP"; break;    // D-pad up
                case 11: keycode = "KEYCODE_DPAD_DOWN"; break;  // D-pad down
                case 12: keycode = "KEYCODE_DPAD_LEFT"; break;  // D-pad left
                case 13: keycode = "KEYCODE_DPAD_RIGHT"; break; // D-pad right
                default: return;
            }

            // Handle analog inputs (sticks and triggers)
            if (event.button >= 14) {
                switch (event.button) {
                    case 14: // Left stick X
                        AdbCommand::execute("shell \"input mouse moveto " + 
                            std::to_string(static_cast<int>((event.value + 1.0f) * screenWidth / 2)) + " " +
                            std::to_string(static_cast<int>(currentY)) + "\"");
                        return;
                    case 15: // Left stick Y
                        AdbCommand::execute("shell \"input mouse moveto " + 
                            std::to_string(static_cast<int>(currentX)) + " " +
                            std::to_string(static_cast<int>((event.value + 1.0f) * screenHeight / 2)) + "\"");
                        return;
                    case 16: // Right stick X
                        // Map to horizontal scroll
                        if (std::abs(event.value) > 0.2f) {
                            AdbCommand::execute("shell input roll " + 
                                std::to_string(static_cast<int>(event.value * 100)));
                        }
                        return;
                    case 17: // Right stick Y
                        // Map to vertical scroll
                        if (std::abs(event.value) > 0.2f) {
                            AdbCommand::execute("shell input roll " + 
                                std::to_string(static_cast<int>(event.value * 100)));
                        }
                        return;
                    case 18: // Left trigger
                    case 19: // Right trigger
                        // Map triggers to volume
                        if (event.value > 0.8f) {
                            AdbCommand::execute("shell input keyevent " + 
                                std::string(event.button == 18 ? "KEYCODE_VOLUME_DOWN" : "KEYCODE_VOLUME_UP"));
                        }
                        return;
                }
            }

            // Send digital button events
            if (event.pressed) {
                AdbCommand::execute("shell input keyevent " + keycode);
            }
        } catch (const utils::Error& e) {
            utils::Logger::getInstance().error("Failed to send gamepad event: ", e.what());
        }
    }

private:
    void initializeDefaultMappings() {
        // Mac keyboard to Android key mappings
        keyMap = {
            {0x35, "KEYCODE_ESCAPE"},     // Escape
            {0x24, "KEYCODE_ENTER"},      // Return
            {0x33, "KEYCODE_DEL"},        // Backspace
            {0x30, "KEYCODE_TAB"},        // Tab
            {0x31, "KEYCODE_SPACE"},      // Space
            {0x7E, "KEYCODE_DPAD_UP"},    // Up Arrow
            {0x7D, "KEYCODE_DPAD_DOWN"},  // Down Arrow
            {0x7B, "KEYCODE_DPAD_LEFT"},  // Left Arrow
            {0x7C, "KEYCODE_DPAD_RIGHT"}, // Right Arrow
            // Add more mappings as needed
        };
    }
    
    std::string escapeString(const std::string& str) {
        std::string result;
        for (char c : str) {
            if (c == '\'' || c == '\\' || c == ' ' || c == '(' || c == ')') {
                result += '\\';
            }
            result += c;
        }
        return result;
    }
    
    std::map<uint32_t, std::string> keyMap;
    float currentX = 0;
    float currentY = 0;
    int screenWidth = 1920;  // Default, should be updated with actual screen size
    int screenHeight = 1080; // Default, should be updated with actual screen size
};

// Public interface implementation
InputHandler::InputHandler() : pimpl(std::make_unique<Impl>()) {}
InputHandler::~InputHandler() = default;

void InputHandler::sendTouchEvent(const TouchEvent& event) {
    pimpl->sendTouchEvent(event);
}

void InputHandler::sendMultiTouchEvents(const std::vector<TouchEvent>& events) {
    for (const auto& event : events) {
        sendTouchEvent(event);
    }
}

void InputHandler::sendKeyEvent(const KeyboardEvent& event) {
    pimpl->sendKeyEvent(event);
}

void InputHandler::sendText(const std::string& text) {
    pimpl->sendText(text);
}

void InputHandler::sendHome() {
    pimpl->sendHome();
}

void InputHandler::sendBack() {
    pimpl->sendBack();
}

void InputHandler::sendAppSwitch() {
    pimpl->sendAppSwitch();
}

void InputHandler::sendVolumeUp() {
    pimpl->sendVolumeUp();
}

void InputHandler::sendVolumeDown() {
    pimpl->sendVolumeDown();
}

void InputHandler::sendVolumeMute() {
    pimpl->sendVolumeMute();
}

void InputHandler::sendPower() {
    pimpl->sendPower();
}

void InputHandler::sendWake() {
    pimpl->sendWake();
}

bool InputHandler::sendClipboardText(const std::string& text) {
    return pimpl->sendClipboardText(text);
}

std::string InputHandler::getDeviceClipboardText() const {
    return pimpl->getDeviceClipboardText();
}

void InputHandler::setInputMapping(const std::string& mappingFile) {
    pimpl->setInputMapping(mappingFile);
}

bool InputHandler::saveInputMapping(const std::string& mappingFile) const {
    return pimpl->saveInputMapping(mappingFile);
}

void InputHandler::sendGamepadEvent(const GamepadEvent& event) {
    pimpl->sendGamepadEvent(event);
}

bool InputHandler::isGamepadConnected() const {
    return pimpl->isGamepadConnected();
}

} // namespace mirrolink