#pragma once

#include <string>
#include <memory>
#include <sstream>
#include <chrono>
#include <source_location>
#include <unordered_map>

namespace mirrolink {
namespace utils {

enum class LogLevel {
    TRACE,  // Added TRACE level for more detailed debugging
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
    void enableConsoleOutput(bool enable);
    void enableFileOutput(bool enable);
    void setMaxFileSize(size_t maxSize);
    void setMaxBackupCount(int count);
    
    // New method for structured logging
    template<typename... Args>
    void logStructured(LogLevel level, 
                      const std::source_location& location, 
                      const Args&... args) {
        if (level >= currentLevel) {
            std::ostringstream ss;
            ss << "[" << location.file_name() << ":" 
               << location.line() << "] ";
            logValue(ss, args...);
            writeLog(level, ss.str());
        }
    }
    
    // Enhanced logging methods with source location
    template<typename... Args>
    void trace(const Args&... args, 
              const std::source_location& location = std::source_location::current()) {
        logStructured(LogLevel::TRACE, location, args...);
    }
    
    template<typename... Args>
    void debug(const Args&... args,
              const std::source_location& location = std::source_location::current()) {
        logStructured(LogLevel::DEBUG, location, args...);
    }
    
    template<typename... Args>
    void info(const Args&... args,
              const std::source_location& location = std::source_location::current()) {
        logStructured(LogLevel::INFO, location, args...);
    }
    
    template<typename... Args>
    void warn(const Args&... args,
              const std::source_location& location = std::source_location::current()) {
        logStructured(LogLevel::WARNING, location, args...);
    }
    
    template<typename... Args>
    void error(const Args&... args,
              const std::source_location& location = std::source_location::current()) {
        logStructured(LogLevel::ERROR, location, args...);
    }
    
    template<typename... Args>
    void fatal(const Args&... args,
              const std::source_location& location = std::source_location::current()) {
        logStructured(LogLevel::FATAL, location, args...);
    }

    // Performance monitoring
    void startPerformanceLog(const std::string& operation);
    void endPerformanceLog(const std::string& operation);

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
    
    void writeLog(LogLevel level, const std::string& message);
    void rotateLogFileIfNeeded();
    std::string getTimestamp() const;
    
    class Impl;
    std::unique_ptr<Impl> pimpl;
    LogLevel currentLevel;
    bool consoleOutput;
    bool fileOutput;
    size_t maxFileSize;
    int maxBackupCount;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> performanceMarkers;
    
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
};

// Helper macro for performance logging
#define PERFORMANCE_SCOPE(name) \
    struct ScopeLogger { \
        std::string opName; \
        ScopeLogger(const std::string& name) : opName(name) { \
            Logger::getInstance().startPerformanceLog(opName); \
        } \
        ~ScopeLogger() { \
            Logger::getInstance().endPerformanceLog(opName); \
        } \
    } scopeLogger(name);