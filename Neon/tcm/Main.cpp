#include "LogFile.h"
#include "Process.h"
#include "transmission/Transmission.h"
#include "shared/can/Receiver.h"
#include "shared/can/Message.h"
#include "shared/can/Bus.h"
#include <atomic>
#include <condition_variable>
#include <csignal>
#include <mutex>
#include <queue>
#include <thread>

std::atomic<bool> running(true);

void signalHandler(int)
{
    LogFile::Info("Stop signal received, shutting down...");
    running = false;
}

int main()
{
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    LogFile::Instance().setLogFile("tcm.log");
    LogFile::Instance().setLevel(LogLevel::DEBUG);

    // Create Transmission object 
    tcm::transmission::Transmission transmission;
    LogFile::Info("Transmission is running!");

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

    tcm::Process process;

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

            switch (msg.getMessageType()) {
            case shared::can::MessageType::GearUpRequest:
                LogFile::Info("Gear up command received.");
                transmission.gearUp();
                break;
            case shared::can::MessageType::GearDownRequest:
                LogFile::Info("Gear down command received.");
                transmission.gearDown();
                break;
            default:
                break;
            }
        }
        });


    // Current gear broadcast thread 
    auto canBus = shared::can::Bus(15000);
    canBus.AddPeer("ecm");
    std::thread gearPublishThread([&]()
        {
            while (running)
            {
                shared::can::Message msg{
                    shared::can::MessageType::CurrentGear,
                    transmission.getGear()
                };

                LogFile::Debug("Publishing CurrentGear=" + std::to_string(transmission.getGear()));
                canBus.Send(msg);

                // sleep ~1s, but allow quicker shutdown
                for (int i = 0; i < 10 && running; ++i)
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        });
    // Main thread just blocks; container stays alive
    processThread.join();

    running = false;
    canRx.Stop();
    cv.notify_all();
    rxThread.join();
    gearPublishThread.join();


    LogFile::Info("All threads stopped cleanly.");
    return 0;
}
