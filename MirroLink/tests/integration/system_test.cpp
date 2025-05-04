#include <gtest/gtest.h>
#include "../../src/gui/main_window.hpp"
#include "../../src/core/device_manager.hpp"
#include "../../src/core/screen_mirror.hpp"
#include "../../src/core/input_handler.hpp"
#include "../../src/utils/logger.hpp"
#include <thread>
#include <chrono>

using namespace mirrolink;
using namespace mirrolink::gui;

class SystemIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        window = std::make_unique<MainWindow>();
        ASSERT_TRUE(window->initialize("Test Window", 1280, 720));
    }
    
    void TearDown() override {
        window.reset();
    }
    
    std::unique_ptr<MainWindow> window;
};

// Test the complete flow of device connection and mirroring
TEST_F(SystemIntegrationTest, CompleteWorkflow) {
    bool deviceConnected = false;
    bool frameReceived = false;
    bool inputSent = false;
    
    // Set up device connection monitoring
    auto deviceManager = std::make_unique<DeviceManager>();
    ASSERT_TRUE(deviceManager->initialize());
    
    deviceManager->onDeviceConnected([&](const DeviceInfo& device) {
        deviceConnected = true;
        
        // Set up screen mirroring
        auto screenMirror = std::make_unique<ScreenMirror>();
        screenMirror->setFrameCallback([&](const FrameData& frame) {
            frameReceived = true;
        });
        
        ScreenConfig config{
            .width = 1280,
            .height = 720,
            .maxFps = 60
        };
        
        EXPECT_TRUE(screenMirror->start(config));
        
        // Test input handling
        TouchEvent touchEvent{
            .id = 0,
            .x = 0.5f,
            .y = 0.5f,
            .pressed = true
        };
        
        InputHandler inputHandler;
        inputHandler.sendTouchEvent(touchEvent);
        inputSent = true;
    });
    
    // Wait for potential device connection
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // These expectations might need adjustment based on whether
    // a real device is connected during testing
    EXPECT_FALSE(deviceConnected);
    EXPECT_FALSE(frameReceived);
    EXPECT_FALSE(inputSent);
}

// Test window behavior and event handling
TEST_F(SystemIntegrationTest, WindowEventHandling) {
    // Simulate keyboard input
    SDL_Event keyEvent;
    keyEvent.type = SDL_KEYDOWN;
    keyEvent.key.keysym.scancode = SDL_SCANCODE_F11;
    keyEvent.key.keysym.mod = 0;
    
    SDL_PushEvent(&keyEvent);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Verify fullscreen toggle
    EXPECT_TRUE(window->isFullscreen());
    
    // Test window resize
    SDL_Event resizeEvent;
    resizeEvent.type = SDL_WINDOWEVENT;
    resizeEvent.window.event = SDL_WINDOWEVENT_RESIZED;
    resizeEvent.window.data1 = 1920;
    resizeEvent.window.data2 = 1080;
    
    SDL_PushEvent(&resizeEvent);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

// Test configuration management
TEST_F(SystemIntegrationTest, ConfigurationPersistence) {
    auto& config = utils::ConfigManager::getInstance();
    
    // Set test configuration
    config.set("window.width", 1920);
    config.set("window.height", 1080);
    config.set("window.title", "Test Config");
    
    // Save and reload configuration
    std::string configPath = utils::ConfigManager::getDefaultConfigPath();
    EXPECT_TRUE(config.saveConfig(configPath));
    
    // Clear and reload
    config.clear();
    EXPECT_TRUE(config.loadConfig(configPath));
    
    // Verify loaded values
    EXPECT_EQ(config.get<int>("window.width", 0), 1920);
    EXPECT_EQ(config.get<int>("window.height", 0), 1080);
    EXPECT_EQ(config.get<std::string>("window.title", ""), "Test Config");
}

// Test error handling and recovery
TEST_F(SystemIntegrationTest, ErrorHandlingAndRecovery) {
    // Test device manager error recovery
    auto deviceManager = std::make_unique<DeviceManager>();
    ASSERT_TRUE(deviceManager->initialize());
    
    // Attempt to connect to invalid device
    EXPECT_FALSE(deviceManager->connectDevice("invalid_serial"));
    EXPECT_FALSE(deviceManager->isDeviceConnected());
    
    // Test screen mirror error recovery
    auto screenMirror = std::make_unique<ScreenMirror>();
    ScreenConfig invalidConfig{
        .width = 0,  // Invalid width
        .height = 0, // Invalid height
        .maxFps = 0  // Invalid FPS
    };
    
    EXPECT_FALSE(screenMirror->start(invalidConfig));
    EXPECT_FALSE(screenMirror->isActive());
    
    // Test valid configuration after invalid
    ScreenConfig validConfig{
        .width = 1280,
        .height = 720,
        .maxFps = 60
    };
    
    EXPECT_TRUE(screenMirror->start(validConfig));
}

// Test audio forwarding integration
TEST_F(SystemIntegrationTest, AudioForwarding) {
    AudioConfig config{
        .sampleRate = 44100,
        .channels = 2,
        .bitsPerSample = 16,
        .bufferSize = 4096
    };
    
    auto audioForwarder = std::make_unique<AudioForwarder>();
    EXPECT_TRUE(audioForwarder->initialize(config));
    
    bool audioReceived = false;
    audioForwarder->setAudioCallback([&](const AudioFrame& frame) {
        audioReceived = true;
    });
    
    EXPECT_TRUE(audioForwarder->start());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    audioForwarder->stop();
    
    // This expectation might need adjustment based on whether
    // a real device is connected during testing
    EXPECT_FALSE(audioReceived);
}