#ifndef LOGFILE_H
#define LOGFILE_H
#ifdef ERROR
#undef ERROR
#endif
#ifdef WARNING
#undef WARNING
#endif
#include <string>
#include <fstream>
#include <chrono>
#include <mutex>

enum class LogLevel 
{
    DEBUG,
    INFO,
    WARNING,
    ERROR,
};

class LogFile 
{
public:
    static LogFile& instance();

    void setLogFile(const std::string& filename);
    void setLevel(LogLevel level);

    void log(LogLevel level, const std::string& message) const;

    static void debug(const std::string& msg);
    static void info(const std::string& msg);
    static void warn(const std::string& msg);
    static void error(const std::string& msg);

private:
    LogFile();
    ~LogFile();

    static std::string timestamp();
    static std::string levelToString(LogLevel level);

    std::ofstream ofs_;
    LogLevel level_;
    mutable std::mutex mutex_;
};


#endif