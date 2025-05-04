#pragma once

#include <memory>
#include <vector>
#include <functional>
#include <cstdint>
#include "input_handler.hpp"

namespace mirrolink {

struct ScreenConfig {
    int width;
    int height;
    int maxFps;
    bool recordAudio = false;
    std::string videoCodec = "h264";
    int videoBitrate = 8000000; // 8 Mbps
};

struct FrameData {
    std::vector<uint8_t> data;
    int width;
    int height;
    int64_t timestamp;
    int format;  // e.g., RGBA, YUV420P
};

class ScreenMirror {
public:
    using FrameCallback = std::function<void(const FrameData&)>;
    
    ScreenMirror();
    ~ScreenMirror();
    
    // Start screen mirroring with given configuration
    bool start(const ScreenConfig& config);
    
    // Stop screen mirroring
    void stop();
    
    // Set callback for receiving frames
    void setFrameCallback(FrameCallback callback);
    
    // Get current configuration
    ScreenConfig getConfig() const;
    
    // Update configuration while running
    bool updateConfig(const ScreenConfig& config);
    
    // Check if mirroring is active
    bool isActive() const;
    
    // Recording functions
    bool startRecording(const std::string& path);
    void stopRecording();
    
    // Get the input handler for this session
    InputHandler& getInputHandler() { return *inputHandler; }
    
private:
    class Impl;
    std::unique_ptr<Impl> pimpl;
    std::unique_ptr<InputHandler> inputHandler;
};
}