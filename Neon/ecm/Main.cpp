#include "LogFile.h"
#include "Process.h"
#include "shared/can/Receiver.h"

#include <atomic>
#include <csignal>
#include <thread>

std::atomic<bool> running(true);

void signalHandler(int)
{
    LogFile::Info("Received stop signal, shutting down...");
    running = false;
}

int main()
{
    LogFile::Instance().setLogFile("ecm.log");
    LogFile::Instance().setLevel(LogLevel::DEBUG);
    LogFile::Info("Engine is running!");

    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    // Start CAN receiver (listen on same port your Dashboard bus sends to)
    shared::can::Receiver canRx(running, /*listenPort=*/15000);

    std::thread rxThread([&]() {
        canRx.Run(); // blocking loop
        });

    ecm::Process process;
    std::thread processThread([&process]() {
        process.run();
        });

    // If your Process has a stop condition, tie it to 'running' too (later).
    // For now, processThread may run forever unless Process exits on its own.

    processThread.join();

    // If Process exits, stop receiver too:
    running = false;
    canRx.Stop();
    rxThread.join();

    LogFile::Info("All threads stopped cleanly.");
    return 0;
}
