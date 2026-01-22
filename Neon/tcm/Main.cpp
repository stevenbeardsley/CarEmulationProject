#include "LogFile.h"
#include "Process.h"
#include <iostream>


int main()
{
    LogFile::Instance().setLogFile("tcm.log");
    LogFile::Instance().setLevel(LogLevel::DEBUG);
    LogFile::Info("Transmission is running!");

    tcm::Process process;
    std::thread processThread([&process]() { process.run(); });

    processThread.join();
    LogFile::Info("All threads stopped cleanly.");

    return 0;
}