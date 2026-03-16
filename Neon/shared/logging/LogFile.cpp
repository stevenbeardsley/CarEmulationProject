#include "LogFile.h"

#include <iomanip>
#include <iostream>
#include <sstream>

LogFile& LogFile::instance()
{
    static LogFile instance;
    return instance;
}

LogFile::LogFile() : level_(LogLevel::DEBUG) {}

LogFile::~LogFile()
{
    if (ofs_.is_open()) ofs_.close();
}

void LogFile::setLogFile(const std::string& filename) 
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (ofs_.is_open())
        ofs_.close();
    ofs_.open(filename, std::ios::app);
    if (!ofs_)
        std::cerr << "SimpleLogFile: Failed to open log file: " << filename << std::endl;
}

void LogFile::setLevel(const LogLevel level)
{
    level_ = level;
}

void LogFile::log(const LogLevel level, const std::string& message) const
{
    if (level < level_)
        return;

    std::ostringstream oss;
    oss << timestamp() << " [" << levelToString(level) << "] " << message << "\n";
    const std::string line = oss.str();

    std::scoped_lock lock(mutex_);

    // 1) Docker logs path (stdout/stderr)
    // Use stderr for errors, stdout otherwise.
    if (level >= LogLevel::ERROR)
    {
        std::cerr << line;
        std::cerr.flush();
    }
    else
    {
        std::cout << line;
        std::cout.flush();
    }
}

void LogFile::debug(const std::string& msg) { instance().log(LogLevel::DEBUG, msg); }
void LogFile::info(const std::string& msg) { instance().log(LogLevel::INFO, msg); }
void LogFile::warn(const std::string& msg) { instance().log(LogLevel::WARNING, msg); }
void LogFile::error(const std::string& msg) { instance().log(LogLevel::ERROR, msg); }

std::string LogFile::timestamp()
{
    const auto now = std::chrono::system_clock::now();
    const auto inTimeT = std::chrono::system_clock::to_time_t(now);
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::ostringstream ss;
    ss << std::put_time(std::localtime(&inTimeT), "%Y-%m-%d %H:%M:%S")
        << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

std::string LogFile::levelToString(const LogLevel level)
{
    switch (level) 
    {
    case LogLevel::DEBUG:   
        return "DEBUG";
    case LogLevel::INFO:    
        return "INFO";
    case LogLevel::WARNING: 
        return "WARN";
    case LogLevel::ERROR:   
        return "ERROR";
    default:                
        return "UNKNOWN";
    }
}

