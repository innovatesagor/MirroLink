#include "logger.hpp"
#include <fstream>
#include <mutex>
#include <ctime>
#include <filesystem>
#include <iomanip>

namespace mirrolink {
namespace utils {

class Logger::Impl {
public:
    void setLogLevel(LogLevel level) {
        currentLevel = level;
    }
    
    void setLogFile(const std::string& path) {
        std::lock_guard<std::mutex> lock(logMutex);
        
        // Create directory if it doesn't exist
        auto dir = std::filesystem::path(path).parent_path();
        std::filesystem::create_directories(dir);
        
        // Open log file in append mode
        logFile.close();
        logFile.open(path, std::ios::app);
        
        if (!logFile.is_open()) {
            std::cerr << "Failed to open log file: " << path << std::endl;
        }
        
        logPath = path;
    }
    
    void writeLog(LogLevel level, const std::string& message) {
        if (level < currentLevel) return;
        
        std::lock_guard<std::mutex> lock(logMutex);
        
        auto now = std::chrono::system_clock::now();
        auto timeT = std::chrono::system_clock::to_time_t(now);
        auto timePoint = std::localtime(&timeT);
        
        std::ostringstream timestamp;
        timestamp << std::put_time(timePoint, "%Y-%m-%d %H:%M:%S");
        
        std::string levelStr;
        switch (level) {
            case LogLevel::DEBUG:   levelStr = "DEBUG"; break;
            case LogLevel::INFO:    levelStr = "INFO"; break;
            case LogLevel::WARNING: levelStr = "WARN"; break;
            case LogLevel::ERROR:   levelStr = "ERROR"; break;
            case LogLevel::FATAL:   levelStr = "FATAL"; break;
        }
        
        std::string logEntry = timestamp.str() + " [" + levelStr + "] " + message + "\n";
        
        // Write to file if opened
        if (logFile.is_open()) {
            logFile << logEntry;
            logFile.flush();
        }
        
        // Also write to console for ERROR and FATAL
        if (level >= LogLevel::ERROR) {
            std::cerr << logEntry;
        }
    }
    
    ~Impl() {
        if (logFile.is_open()) {
            logFile.close();
        }
    }

private:
    std::mutex logMutex;
    std::ofstream logFile;
    std::string logPath;
    LogLevel currentLevel{LogLevel::INFO};
};

// Static instance
Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

// Constructor/Destructor
Logger::Logger() : pimpl(std::make_unique<Impl>()) {}
Logger::~Logger() = default;

// Public interface implementation
void Logger::setLogLevel(LogLevel level) {
    pimpl->setLogLevel(level);
}

void Logger::setLogFile(const std::string& path) {
    pimpl->setLogFile(path);
}

void Logger::writeLog(LogLevel level, const std::string& message) {
    pimpl->writeLog(level, message);
}

}} // namespace mirrolink::utils