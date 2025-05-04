#pragma once

#include <string>
#include <memory>
#include <sstream>

namespace mirrolink {
namespace utils {

enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    FATAL
};

class Logger {
public:
    static Logger& getInstance();

    void setLogLevel(LogLevel level);
    void setLogFile(const std::string& path);
    
    template<typename... Args>
    void debug(const Args&... args) {
        log(LogLevel::DEBUG, args...);
    }
    
    template<typename... Args>
    void info(const Args&... args) {
        log(LogLevel::INFO, args...);
    }
    
    template<typename... Args>
    void warn(const Args&... args) {
        log(LogLevel::WARNING, args...);
    }
    
    template<typename... Args>
    void error(const Args&... args) {
        log(LogLevel::ERROR, args...);
    }
    
    template<typename... Args>
    void fatal(const Args&... args) {
        log(LogLevel::FATAL, args...);
    }

private:
    Logger();
    ~Logger();
    
    template<typename T>
    void logValue(std::ostringstream& ss, const T& value) {
        ss << value;
    }
    
    template<typename First, typename... Rest>
    void logValue(std::ostringstream& ss, const First& first, const Rest&... rest) {
        ss << first;
        logValue(ss, rest...);
    }
    
    template<typename... Args>
    void log(LogLevel level, const Args&... args) {
        if (level >= currentLevel) {
            std::ostringstream ss;
            logValue(ss, args...);
            writeLog(level, ss.str());
        }
    }
    
    void writeLog(LogLevel level, const std::string& message);
    
    class Impl;
    std::unique_ptr<Impl> pimpl;
    LogLevel currentLevel;
    
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
};