#pragma once

#include <memory>
#include <string>
#include <vector>
#include <functional>

namespace mirrolink {

struct DeviceInfo {
    std::string serial;
    std::string model;
    std::string manufacturer;
    int api_level;
    bool authorized;
};

class DeviceManager {
public:
    using DeviceCallback = std::function<void(const DeviceInfo&)>;
    
    DeviceManager();
    ~DeviceManager();

    // Initialize ADB connection and start device monitoring
    bool initialize();
    
    // Get list of connected devices
    std::vector<DeviceInfo> getConnectedDevices();
    
    // Connect to a specific device
    bool connectDevice(const std::string& serial);
    
    // Disconnect from current device
    void disconnectDevice();
    
    // Register callbacks for device events
    void onDeviceConnected(DeviceCallback callback);
    void onDeviceDisconnected(DeviceCallback callback);
    
    // Check if a device is currently connected
    bool isDeviceConnected() const;
    
    // Get current device info
    DeviceInfo getCurrentDevice() const;

private:
    class Impl;
    std::unique_ptr<Impl> pimpl;
};