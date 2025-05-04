#include "device_manager.hpp"
#include "../utils/logger.hpp"
#include "../utils/error.hpp"
#include <libusb-1.0/libusb.h>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <optional>

namespace mirrolink {

class DeviceManager::Impl {
public:
    Impl() : initialized(false), monitoring(false) {}
    
    bool initialize() {
        PERFORMANCE_SCOPE("DeviceManager::Initialize");
        
        try {
            if (libusb_init(nullptr) != 0) {
                utils::Logger::getInstance().error("Failed to initialize libusb: ", libusb_strerror(static_cast<libusb_error>(ret)));
                return false;
            }
            initialized = true;

            // Test ADB connection
            try {
                AdbCommand::execute("devices", false);
            } catch (const utils::Error& e) {
                utils::Logger::getInstance().error("ADB not available or not properly set up: ", e.what());
                utils::Logger::getInstance().info("Please ensure Android SDK Platform Tools are installed and 'adb' is in PATH");
                return false;
            }

            startMonitoring();
            utils::Logger::getInstance().info("Device manager initialized successfully");
            return true;
        } catch (const std::exception& e) {
            utils::Logger::getInstance().error("Unexpected error during device manager initialization: ", e.what());
            return false;
        }
    }
    
    ~Impl() {
        stopMonitoring();
        if (initialized) {
            libusb_exit(nullptr);
        }
    }

    void onDeviceConnected(DeviceCallback callback) {
        std::lock_guard<std::mutex> lock(callbackMutex);
        connectedCallback = callback;
    }
    
    void onDeviceDisconnected(DeviceCallback callback) {
        std::lock_guard<std::mutex> lock(callbackMutex);
        disconnectedCallback = callback;
    }
    
    std::vector<DeviceInfo> getConnectedDevices() {
        std::lock_guard<std::mutex> lock(devicesMutex);
        return connectedDevices;
    }
    
    bool connectDevice(const std::string& serial) {
        std::lock_guard<std::mutex> lock(devicesMutex);
        for (const auto& device : connectedDevices) {
            if (device.serial == serial) {
                currentDevice = device;
                return true;
            }
        }
        return false;
    }
    
    void disconnectDevice() {
        std::lock_guard<std::mutex> lock(devicesMutex);
        currentDevice = std::nullopt;
    }
    
    bool isDeviceConnected() const {
        std::lock_guard<std::mutex> lock(devicesMutex);
        return currentDevice.has_value();
    }
    
    DeviceInfo getCurrentDevice() const {
        std::lock_guard<std::mutex> lock(devicesMutex);
        if (!currentDevice) {
            utils::throwSystemError("No device connected");
        }
        return *currentDevice;
    }

private:
    void startMonitoring() {
        if (monitoring) return;
        
        monitoring = true;
        monitorThread = std::thread([this]() {
            while (monitoring) {
                checkDevices();
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        });
    }
    
    void stopMonitoring() {
        if (!monitoring) return;
        
        monitoring = false;
        if (monitorThread.joinable()) {
            monitorThread.join();
        }
    }
    
    void checkDevices() {
        libusb_device **devices;
        ssize_t count = libusb_get_device_list(nullptr, &devices);
        
        if (count < 0) {
            utils::Logger::getInstance().error("Failed to get USB device list");
            return;
        }
        
        std::vector<DeviceInfo> currentDevices;
        for (ssize_t i = 0; i < count; i++) {
            processDevice(devices[i], currentDevices);
        }
        
        updateConnectedDevices(currentDevices);
        libusb_free_device_list(devices, 1);
    }
    
    void processDevice(libusb_device* device, std::vector<DeviceInfo>& currentDevices) {
        libusb_device_descriptor desc;
        if (libusb_get_device_descriptor(device, &desc) != 0) {
            return;
        }
        
        // Check if this is an Android device
        if (isAndroidDevice(desc)) {
            DeviceInfo info;
            if (getDeviceInfo(device, desc, info)) {
                currentDevices.push_back(info);
            }
        }
    }
    
    bool isAndroidDevice(const libusb_device_descriptor& desc) {
        // Common Android vendor IDs
        static const std::vector<uint16_t> androidVendors = {
            0x18d1,  // Google
            0x04e8,  // Samsung
            0x22b8,  // Motorola
            0x2717,  // Xiaomi
            // Add more vendor IDs as needed
        };
        
        return std::find(androidVendors.begin(), androidVendors.end(), 
                        desc.idVendor) != androidVendors.end();
    }
    
    bool getDeviceInfo(libusb_device* device, 
                      const libusb_device_descriptor& desc,
                      DeviceInfo& info) {
        libusb_device_handle* handle;
        if (libusb_open(device, &handle) != 0) {
            return false;
        }
        
        unsigned char serial[256];
        if (libusb_get_string_descriptor_ascii(handle, desc.iSerialNumber,
                                             serial, sizeof(serial)) > 0) {
            info.serial = reinterpret_cast<char*>(serial);
        }
        
        // Get manufacturer name
        unsigned char manufacturer[256];
        if (libusb_get_string_descriptor_ascii(handle, desc.iManufacturer,
                                             manufacturer, sizeof(manufacturer)) > 0) {
            info.manufacturer = reinterpret_cast<char*>(manufacturer);
        }
        
        // Get product name
        unsigned char product[256];
        if (libusb_get_string_descriptor_ascii(handle, desc.iProduct,
                                             product, sizeof(product)) > 0) {
            info.model = reinterpret_cast<char*>(product);
        }
        
        libusb_close(handle);
        return true;
    }
    
    void updateConnectedDevices(const std::vector<DeviceInfo>& currentDevices) {
        std::lock_guard<std::mutex> lock(devicesMutex);
        std::lock_guard<std::mutex> callbackLock(callbackMutex);
        
        // Check for disconnected devices
        for (const auto& existing : connectedDevices) {
            bool stillConnected = false;
            for (const auto& current : currentDevices) {
                if (existing.serial == current.serial) {
                    stillConnected = true;
                    break;
                }
            }
            if (!stillConnected) {
                utils::Logger::getInstance().info("Device disconnected: ", existing.serial);
                if (disconnectedCallback) {
                    disconnectedCallback(existing);
                }
                if (currentDevice && currentDevice->serial == existing.serial) {
                    currentDevice = std::nullopt;
                }
            }
        }
        
        // Check for new devices
        for (const auto& current : currentDevices) {
            bool isNew = true;
            for (const auto& existing : connectedDevices) {
                if (current.serial == existing.serial) {
                    isNew = false;
                    break;
                }
            }
            if (isNew) {
                utils::Logger::getInstance().info("New device connected: ", current.serial);
                if (connectedCallback) {
                    connectedCallback(current);
                }
            }
        }
        
        connectedDevices = currentDevices;
    }
    
    bool initialized;
    bool monitoring;
    std::thread monitorThread;
    std::mutex devicesMutex;
    std::mutex callbackMutex;
    std::vector<DeviceInfo> connectedDevices;
    std::optional<DeviceInfo> currentDevice;
    DeviceCallback connectedCallback;
    DeviceCallback disconnectedCallback;
};

// Public interface implementation
DeviceManager::DeviceManager() : pimpl(std::make_unique<Impl>()) {}
DeviceManager::~DeviceManager() = default;

bool DeviceManager::initialize() {
    return pimpl->initialize();
}

std::vector<DeviceInfo> DeviceManager::getConnectedDevices() {
    return pimpl->getConnectedDevices();
}

bool DeviceManager::connectDevice(const std::string& serial) {
    return pimpl->connectDevice(serial);
}

void DeviceManager::disconnectDevice() {
    pimpl->disconnectDevice();
}

bool DeviceManager::isDeviceConnected() const {
    return pimpl->isDeviceConnected();
}

DeviceInfo DeviceManager::getCurrentDevice() const {
    return pimpl->getCurrentDevice();
}

void DeviceManager::onDeviceConnected(DeviceCallback callback) {
    pimpl->onDeviceConnected(callback);
}

void DeviceManager::onDeviceDisconnected(DeviceCallback callback) {
    pimpl->onDeviceDisconnected(callback);
}

} // namespace mirrolink