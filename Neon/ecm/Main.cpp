#include "LogFile.h"
#include "Process.h"
#include <iostream>


int main()
{
    LogFile::Instance().setLogFile("ecm.log");
    LogFile::Instance().setLevel(LogLevel::DEBUG);
    LogFile::Info("Engine is running!");

    ecm::Process process;
    std::thread processThread([&process]() { process.run(); });

    processThread.join();
    LogFile::Info("All threads stopped cleanly.");

    return 0;
}