#pragma once

#include <string>
#include <exception>
#include <system_error>

namespace mirrolink {
namespace utils {

class Error : public std::exception {
public:
    explicit Error(const std::string& message) : msg_(message) {}
    explicit Error(const char* message) : msg_(message) {}
    virtual ~Error() noexcept = default;

    virtual const char* what() const noexcept override {
        return msg_.c_str();
    }

protected:
    std::string msg_;
};

class DeviceError : public Error {
public:
    using Error::Error;
};

class ConnectionError : public Error {
public:
    using Error::Error;
};

class ConfigurationError : public Error {
public:
    using Error::Error;
};

// Helper function to check conditions and throw errors
template<typename ErrorType = Error>
inline void throwIfFailed(bool condition, const std::string& message) {
    if (!condition) {
        throw ErrorType(message);
    }
}

// Helper for system errors
inline void throwSystemError(const std::string& message) {
    throw std::system_error(
        std::error_code(errno, std::system_category()),
        message
    );
}

}} // namespace mirrolink::utils