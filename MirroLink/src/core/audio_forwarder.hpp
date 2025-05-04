#pragma once

#include <memory>
#include <functional>
#include <vector>
#include <cstdint>

namespace mirrolink {

struct AudioConfig {
    int sampleRate = 44100;
    int channels = 2;
    int bitsPerSample = 16;
    int bufferSize = 4096;
};

struct AudioFrame {
    std::vector<uint8_t> data;
    int64_t timestamp;
    int sampleCount;
};

class AudioForwarder {
public:
    using AudioCallback = std::function<void(const AudioFrame&)>;
    
    AudioForwarder();
    ~AudioForwarder();

    // Initialize audio forwarding with given configuration
    bool initialize(const AudioConfig& config);
    
    // Start/Stop audio forwarding
    bool start();
    void stop();
    
    // Set audio frame callback
    void setAudioCallback(AudioCallback callback);
    
    // Audio device management
    std::vector<std::string> getAvailableDevices() const;
    bool setOutputDevice(const std::string& deviceName);
    
    // Volume control
    void setVolume(float volume); // 0.0 to 1.0
    float getVolume() const;
    
    // Mute control
    void setMute(bool muted);
    bool isMuted() const;
    
    // Status
    bool isActive() const;
    AudioConfig getCurrentConfig() const;

private:
    class Impl;
    std::unique_ptr<Impl> pimpl;
};