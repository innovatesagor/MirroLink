#include "audio_forwarder.hpp"
#include "../utils/logger.hpp"
#include "../utils/error.hpp"
#include <SDL2/SDL.h>
#include <thread>
#include <queue>
#include <mutex>

namespace mirrolink {

class AudioForwarder::Impl {
public:
    Impl() : active(false), muted(false), volume(1.0f) {}
    
    bool initialize(const AudioConfig& config) {
        if (SDL_Init(SDL_INIT_AUDIO) < 0) {
            utils::Logger::getInstance().error("Failed to initialize SDL audio: ", SDL_GetError());
            return false;
        }
        
        currentConfig = config;
        
        SDL_AudioSpec desired;
        SDL_zero(desired);
        desired.freq = config.sampleRate;
        desired.format = AUDIO_S16SYS;
        desired.channels = config.channels;
        desired.samples = config.bufferSize;
        desired.callback = &audioCallback;
        desired.userdata = this;
        
        deviceId = SDL_OpenAudioDevice(nullptr, 0, &desired, &obtained, 0);
        if (deviceId == 0) {
            utils::Logger::getInstance().error("Failed to open audio device: ", SDL_GetError());
            return false;
        }
        
        return true;
    }
    
    bool start() {
        if (active) return false;
        
        active = true;
        SDL_PauseAudioDevice(deviceId, 0);
        return true;
    }
    
    void stop() {
        if (!active) return;
        
        active = false;
        SDL_PauseAudioDevice(deviceId, 1);
        
        std::lock_guard<std::mutex> lock(queueMutex);
        while (!audioQueue.empty()) {
            audioQueue.pop();
        }
    }
    
    void setVolume(float vol) {
        volume = std::max(0.0f, std::min(1.0f, vol));
    }
    
    float getVolume() const {
        return volume;
    }
    
    void setMute(bool m) {
        muted = m;
    }
    
    bool isMuted() const {
        return muted;
    }
    
    void queueAudio(const AudioFrame& frame) {
        if (!active) return;
        
        std::lock_guard<std::mutex> lock(queueMutex);
        audioQueue.push(frame);
        
        if (audioQueue.size() > 10) { // Prevent buffer overflow
            audioQueue.pop();
        }
    }
    
    std::vector<std::string> getAvailableDevices() const {
        std::vector<std::string> devices;
        int count = SDL_GetNumAudioDevices(0);
        for (int i = 0; i < count; i++) {
            const char* name = SDL_GetAudioDeviceName(i, 0);
            if (name) {
                devices.push_back(name);
            }
        }
        return devices;
    }
    
    bool setOutputDevice(const std::string& deviceName) {
        // Stop current playback
        stop();
        
        // Close current device
        if (deviceId != 0) {
            SDL_CloseAudioDevice(deviceId);
        }
        
        // Open new device
        SDL_AudioSpec desired;
        SDL_zero(desired);
        desired.freq = currentConfig.sampleRate;
        desired.format = AUDIO_S16SYS;
        desired.channels = currentConfig.channels;
        desired.samples = currentConfig.bufferSize;
        desired.callback = &audioCallback;
        desired.userdata = this;
        
        deviceId = SDL_OpenAudioDevice(
            deviceName.empty() ? nullptr : deviceName.c_str(),
            0, &desired, &obtained, 0
        );
        
        if (deviceId == 0) {
            utils::Logger::getInstance().error("Failed to open audio device: ", SDL_GetError());
            return false;
        }
        
        // Restart playback if it was active
        if (active) {
            start();
        }
        
        return true;
    }

private:
    static void audioCallback(void* userdata, Uint8* stream, int len) {
        auto* impl = static_cast<Impl*>(userdata);
        impl->fillAudioBuffer(stream, len);
    }
    
    void fillAudioBuffer(Uint8* stream, int len) {
        std::lock_guard<std::mutex> lock(queueMutex);
        
        if (audioQueue.empty() || muted) {
            SDL_memset(stream, 0, len);
            return;
        }
        
        AudioFrame& frame = audioQueue.front();
        int bytesToCopy = std::min(len, static_cast<int>(frame.data.size()));
        
        if (volume == 1.0f) {
            SDL_memcpy(stream, frame.data.data(), bytesToCopy);
        } else {
            // Apply volume scaling
            auto* src = reinterpret_cast<const Sint16*>(frame.data.data());
            auto* dst = reinterpret_cast<Sint16*>(stream);
            int samples = bytesToCopy / 2;
            
            for (int i = 0; i < samples; i++) {
                dst[i] = static_cast<Sint16>(src[i] * volume);
            }
        }
        
        if (bytesToCopy >= static_cast<int>(frame.data.size())) {
            audioQueue.pop();
        } else {
            // Keep remaining data for next callback
            frame.data.erase(frame.data.begin(), frame.data.begin() + bytesToCopy);
        }
    }
    
    std::atomic<bool> active;
    std::atomic<bool> muted;
    std::atomic<float> volume;
    SDL_AudioDeviceID deviceId{0};
    SDL_AudioSpec obtained;
    AudioConfig currentConfig;
    
    std::mutex queueMutex;
    std::queue<AudioFrame> audioQueue;
};

// Public interface implementation
AudioForwarder::AudioForwarder() : pimpl(std::make_unique<Impl>()) {}
AudioForwarder::~AudioForwarder() = default;

bool AudioForwarder::initialize(const AudioConfig& config) {
    return pimpl->initialize(config);
}

bool AudioForwarder::start() {
    return pimpl->start();
}

void AudioForwarder::stop() {
    pimpl->stop();
}

void AudioForwarder::setVolume(float volume) {
    pimpl->setVolume(volume);
}

float AudioForwarder::getVolume() const {
    return pimpl->getVolume();
}

void AudioForwarder::setMute(bool muted) {
    pimpl->setMute(muted);
}

bool AudioForwarder::isMuted() const {
    return pimpl->isMuted();
}

std::vector<std::string> AudioForwarder::getAvailableDevices() const {
    return pimpl->getAvailableDevices();
}

bool AudioForwarder::setOutputDevice(const std::string& deviceName) {
    return pimpl->setOutputDevice(deviceName);
}

bool AudioForwarder::isActive() const {
    return pimpl->active;
}

AudioConfig AudioForwarder::getCurrentConfig() const {
    return pimpl->currentConfig;
}

} // namespace mirrolink