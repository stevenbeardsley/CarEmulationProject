#include "LogFile.h"
#include "Process.h"
#include "shared/can/Receiver.h"

#include <atomic>
#include <csignal>
#include <thread>
#include <queue>

std::atomic<bool> running(true);

void signalHandler(int)
{
    LogFile::Info("Received stop signal, shutting down...");
    running = false;
}

int main()
{
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    LogFile::Instance().setLogFile("ecm.log");
    LogFile::Instance().setLevel(LogLevel::DEBUG);
    LogFile::Info("Engine is running!");

    std::mutex m;
    std::condition_variable cv;
    std::queue<shared::can::Message> inbox;

    shared::can::Receiver canRx(running, /*listenPort=*/15000);

    // Receiver thread: enqueue + notify
    canRx.SetHandler([&](shared::can::Message msg, const auto& sender) {
        (void)sender;
        {
            std::lock_guard<std::mutex> lk(m);
            inbox.push(std::move(msg));
        }
        cv.notify_one();
        });

    std::thread rxThread([&]() {
        canRx.Run(); // blocking loop inside receiver
        });

    ecm::Process process;

    // Process thread: wait -> pop -> act
    std::thread processThread([&]() {
        while (running) {
            shared::can::Message msg = [&]() {
                std::unique_lock<std::mutex> lk(m);
                cv.wait(lk, [&]() { return !running || !inbox.empty(); });

                // shutdown path
                if (!running && inbox.empty()) {
                    // return *something* unreachable; we'll break above
                    // but keep structure simple:
                    // (we'll handle break outside)
                }

                if (inbox.empty()) {
                    // running became false
                    return shared::can::Message(shared::can::MessageType::RPM, 0);
                }

                auto m0 = std::move(inbox.front());
                inbox.pop();
                return m0;
                }();

            if (!running) break;

            switch (msg.getMessageType())
            {
            case shared::can::MessageType::Gear:
                LogFile::Info("Gear command received - not subscribed .");
                // process.onGearUp(); or onGearDown based on msg.getValue()
                break;

            default:
                break;
            }
        }
        });

    // Main thread just blocks; container stays alive
    processThread.join();

    running = false;
    canRx.Stop();
    cv.notify_all();
    rxThread.join();

    LogFile::Info("All threads stopped cleanly.");
    return 0;
}
