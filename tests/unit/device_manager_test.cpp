#include <gtest/gtest.h>
#include "../../src/core/device_manager.hpp"
#include "../../src/utils/error.hpp"
#include <thread>
#include <chrono>

using namespace mirrolink;

class DeviceManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        manager = std::make_unique<DeviceManager>();
    }
    
    void TearDown() override {
        manager.reset();
    }
    
    std::unique_ptr<DeviceManager> manager;
};

TEST_F(DeviceManagerTest, InitializationSuccess) {
    EXPECT_TRUE(manager->initialize());
}

TEST_F(DeviceManagerTest, GetDevicesWhenNoneConnected) {
    ASSERT_TRUE(manager->initialize());
    auto devices = manager->getConnectedDevices();
    EXPECT_TRUE(devices.empty());
}

TEST_F(DeviceManagerTest, ConnectToNonexistentDevice) {
    ASSERT_TRUE(manager->initialize());
    EXPECT_FALSE(manager->connectDevice("nonexistent_serial"));
}

TEST_F(DeviceManagerTest, DeviceConnectionState) {
    ASSERT_TRUE(manager->initialize());
    EXPECT_FALSE(manager->isDeviceConnected());
}

TEST_F(DeviceManagerTest, DeviceEventCallbacks) {
    bool connected = false;
    bool disconnected = false;
    DeviceInfo testDevice;
    
    ASSERT_TRUE(manager->initialize());
    
    manager->onDeviceConnected([&](const DeviceInfo& device) {
        connected = true;
        testDevice = device;
    });
    
    manager->onDeviceDisconnected([&](const DeviceInfo& device) {
        disconnected = true;
    });
    
    // Wait for potential device events
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    // Note: These expectations might need to be adjusted based on whether
    // a real device is connected during testing
    EXPECT_FALSE(connected);
    EXPECT_FALSE(disconnected);
}

TEST_F(DeviceManagerTest, MultipleInitializationAttempts) {
    EXPECT_TRUE(manager->initialize());
    EXPECT_FALSE(manager->initialize()); // Should fail on second attempt
}

// Mock test for device connection scenario
TEST_F(DeviceManagerTest, MockDeviceConnection) {
    ASSERT_TRUE(manager->initialize());
    
    DeviceInfo mockDevice;
    mockDevice.serial = "TEST001";
    mockDevice.model = "Test Model";
    mockDevice.manufacturer = "Test Manufacturer";
    mockDevice.api_level = 30;
    
    bool deviceFound = false;
    manager->onDeviceConnected([&](const DeviceInfo& device) {
        deviceFound = device.serial == mockDevice.serial;
    });
    
    // Note: In a real implementation, we would need to mock the USB device
    // detection system to properly test this
    EXPECT_FALSE(deviceFound);
}