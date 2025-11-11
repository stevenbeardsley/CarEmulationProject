#pragma once
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <mutex>


enum class LogLevel 
{
    DEBUG,
    INFO,
    WARNING,
    ERROR
};

class LogFile 
{
public:
    static LogFile& Instance();

    void setLogFile(const std::string& filename);
    void setLevel(LogLevel level);

    void log(LogLevel level, const std::string& message);

    static void Debug(const std::string& msg);
    static void Info(const std::string& msg);
    static void Warn(const std::string& msg);
    static void Error(const std::string& msg);

private:
    LogFile();
    ~LogFile();

    std::string timestamp() const;
    std::string levelToString(LogLevel level) const;

    std::ofstream ofs_;
    LogLevel level_;
    mutable std::mutex mutex_;
};

