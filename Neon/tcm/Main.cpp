#include "LogFile.h"
#include "Process.h"
#include "transmission/Transmission.h"
#include "shared/can/Receiver.h"
#include "shared/can/Bus.h"
#include "shared/can/MessageCategory.h"
#include "shared/can/messages/StatusMessage.h"
#include "shared/can/messages/CommandMessage.h"
#include "shared/can/headers/Status.h"
#include "shared/can/headers/Control.h"
#include "shared/bit_parser/BitReader.h"
#include "shared/config/Config.h"

#include <atomic>
#include <condition_variable>
#include <csignal>
#include <mutex>
#include <queue>
#include <thread>
#include <chrono>

std::atomic<bool> running(true);

void static signalHandler(int)
{
    LogFile::info("Stop signal received, shutting down...");
    running = false;
}

int main()
{
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    LogFile::instance().setLogFile("tcm.log");
    LogFile::instance().setLevel(LogLevel::DEBUG);

    shared::config::Config config;
    config.LoadFromFile("config.json");

    const auto& engineConfig = config.getEngineConfig();
    const auto& transmissionConfig = config.getTransmissionConfig();

    tcm::transmission::Transmission transmission{ engineConfig, transmissionConfig };
    LogFile::info("Transmission is running!");

    std::mutex m;
    std::condition_variable cv;
    std::queue<std::vector<std::uint8_t>> inbox;

    shared::can::Receiver canRx(running, 15000, inbox, m, cv);

    std::thread rxThread([&]() {
        canRx.Run();
        });

    std::thread processThread([&]() {
        while (running) {
            std::vector<std::uint8_t> rawData;
            {
                std::unique_lock<std::mutex> lk(m);
                cv.wait(lk, [&]() { return !running || !inbox.empty(); });

                if (!running && inbox.empty()) break;

                rawData = std::move(inbox.front());
                inbox.pop();
            }

            if (rawData.empty()) continue;

            // Use BitReader to peek at headers
            shared::bit_parser::BitReader reader(
                shared::bit_parser::Span<const std::uint8_t>(rawData.data(), rawData.size())
            );

            auto category = static_cast<shared::can::MessageCategory>(reader.readU8());
            auto typeHeader = reader.readU8();

            if (category == shared::can::MessageCategory::Control) {
                switch (static_cast<shared::can::headers::Control>(typeHeader)) {
                case shared::can::headers::Control::GearUpRequest:
                    LogFile::info("TCM: Gear up command received.");
                    transmission.gearUp();
                    break;
                case shared::can::headers::Control::GearDownRequest:
                    LogFile::info("TCM: Gear down command received.");
                    transmission.gearDown();
                    break;
                default:
                    break;
                }
            }
        }
        });

    shared::can::Bus canBus(0, 15000);
    (void)canBus.addPeer("ecm", 15000);
    (void)canBus.addPeer("dashboard", 15000);

    std::thread gearPublishThread([&]() {
        while (running) {
            shared::can::message::StatusMessage msg{
                shared::can::MessageCategory::Status,
                shared::can::headers::Status::CurrentGear,
                static_cast<std::uint32_t>(transmission.getGear())
            };

            LogFile::debug("TCM: Publishing CurrentGear=" + std::to_string(transmission.getGear()));
            canBus.send(msg);

            // Shutdown-aware sleep
            for (auto i = 0; i < 10 && running; ++i)
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        });

    processThread.join();

    // Shutdown order
    running = false;
    canRx.Stop();
    cv.notify_all();

    if (rxThread.joinable()) rxThread.join();
    if (gearPublishThread.joinable()) gearPublishThread.join();

    LogFile::info("All threads stopped cleanly.");
    return 0;
}