#include "logger.hpp"
#include <fstream>
#include <iostream>
#include <iomanip>
#include <filesystem>
#include <mutex>

namespace fs = std::filesystem;

namespace mirrolink {
namespace utils {

class Logger::Impl {
public:
    Impl() : logStream(nullptr) {}
    
    ~Impl() {
        if (logStream) {
            logStream->close();
            delete logStream;
        }
    }
    
    std::ofstream* logStream;
    std::mutex logMutex;
};

Logger::Logger()
    : pimpl(std::make_unique<Impl>())
    , currentLevel(LogLevel::INFO)
    , consoleOutput(true)
    , fileOutput(true)
    , maxFileSize(10 * 1024 * 1024)  // 10MB default
    , maxBackupCount(5)
{}

Logger::~Logger() = default;

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

void Logger::setLogLevel(LogLevel level) {
    currentLevel = level;
}

void Logger::setLogFile(const std::string& path) {
    std::lock_guard<std::mutex> lock(pimpl->logMutex);
    
    if (pimpl->logStream) {
        pimpl->logStream->close();
        delete pimpl->logStream;
    }
    
    pimpl->logStream = new std::ofstream(path, std::ios::app);
    if (!pimpl->logStream->is_open()) {
        std::cerr << "Failed to open log file: " << path << std::endl;
        delete pimpl->logStream;
        pimpl->logStream = nullptr;
    }
}

void Logger::enableConsoleOutput(bool enable) {
    consoleOutput = enable;
}

void Logger::enableFileOutput(bool enable) {
    fileOutput = enable;
}

void Logger::setMaxFileSize(size_t maxSize) {
    maxFileSize = maxSize;
}

void Logger::setMaxBackupCount(int count) {
    maxBackupCount = count;
}

void Logger::startPerformanceLog(const std::string& operation) {
    performanceMarkers[operation] = std::chrono::steady_clock::now();
}

void Logger::endPerformanceLog(const std::string& operation) {
    auto it = performanceMarkers.find(operation);
    if (it != performanceMarkers.end()) {
        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - it->second);
        performanceMarkers.erase(it);
        
        debug("Performance: ", operation, " took ", duration.count(), "ms");
    }
}

std::string Logger::getTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()
    ).count() % 1000;
    
    std::ostringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S")
       << '.' << std::setfill('0') << std::setw(3) << ms;
    return ss.str();
}

void Logger::rotateLogFileIfNeeded() {
    if (!pimpl->logStream || !fileOutput) return;
    
    pimpl->logStream->flush();
    size_t size = pimpl->logStream->tellp();
    
    if (size >= maxFileSize) {
        std::string logPath = pimpl->logStream->getloc().name();
        pimpl->logStream->close();
        
        // Rotate existing backup logs
        for (int i = maxBackupCount - 1; i >= 1; --i) {
            std::string oldName = logPath + "." + std::to_string(i);
            std::string newName = logPath + "." + std::to_string(i + 1);
            if (fs::exists(oldName)) {
                fs::rename(oldName, newName);
            }
        }
        
        // Backup current log
        fs::rename(logPath, logPath + ".1");
        
        // Create new log file
        pimpl->logStream->open(logPath, std::ios::app);
    }
}

void Logger::writeLog(LogLevel level, const std::string& message) {
    static const char* levelStrings[] = {
        "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"
    };
    
    std::lock_guard<std::mutex> lock(pimpl->logMutex);
    
    std::ostringstream ss;
    ss << getTimestamp() << " [" << levelStrings[static_cast<int>(level)] << "] "
       << message << std::endl;
    
    if (consoleOutput) {
        if (level >= LogLevel::WARNING) {
            std::cerr << ss.str();
        } else {
            std::cout << ss.str();
        }
    }
    
    if (fileOutput && pimpl->logStream) {
        *pimpl->logStream << ss.str();
        pimpl->logStream->flush();
        rotateLogFileIfNeeded();
    }
}

}} // namespace mirrolink::utils