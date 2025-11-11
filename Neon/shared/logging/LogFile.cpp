#include "LogFile.h"

LogFile& LogFile::Instance()
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

void LogFile::setLevel(LogLevel level)
{
    level_ = level;
}

void LogFile::log(LogLevel level, const std::string& message)
{
    if (level < level_) return;

    std::ostringstream oss;
    oss << timestamp() << " [" << levelToString(level) << "] " << message << "\n";

    std::lock_guard<std::mutex> lock(mutex_);
    if (ofs_.is_open())
    {
        ofs_ << oss.str();
        ofs_.flush();
    }
    else 
    {
        std::cerr << oss.str();
    }
}

void LogFile::Debug(const std::string& msg) { Instance().log(LogLevel::DEBUG, msg); }
void LogFile::Info(const std::string& msg) { Instance().log(LogLevel::INFO, msg); }
void LogFile::Warn(const std::string& msg) { Instance().log(LogLevel::WARNING, msg); }
void LogFile::Error(const std::string& msg) { Instance().log(LogLevel::ERROR, msg); }

std::string LogFile::timestamp() const 
{
    const auto now = std::chrono::system_clock::now();
    const auto in_time_t = std::chrono::system_clock::to_time_t(now);
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::ostringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S")
        << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

std::string LogFile::levelToString(LogLevel level) const
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

