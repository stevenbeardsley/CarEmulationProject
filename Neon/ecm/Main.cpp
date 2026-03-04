#include "LogFile.h"
#include "Process.h"
#include "shared/can/Receiver.h"
#include "shared/can/Bus.h"
// Updated includes to match your new message structure
#include "shared/can/messages/StatusMessage.h"
#include "shared/can/messages/ErrorMessage.h"
#include "shared/can/messages/Throttle.h"
#include "shared/can/headers/Status.h"
#include "shared/can/headers/Control.h"
#include "shared/can/headers/Error.h"
#include "shared/bit_parser/BitReader.h"

#include "shared/FastUpdate.h"
#include "shared/SlowUpdate.h"
#include "ecm/engine/Engine.h"
#include "shared/config/Config.h"

#include <atomic>
#include <csignal>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <cmath>

std::atomic<bool> running(true);

void signalHandler(int) {
    LogFile::Info("Received stop signal, shutting down...");
    running = false;
}

static inline std::int32_t scaleTemp(double tempC) {
    return static_cast<std::int32_t>(std::llround(tempC * 10.0));
}

int main() {
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    LogFile::Instance().setLogFile("ecm.log");
    LogFile::Instance().setLevel(LogLevel::DEBUG);
    LogFile::Info("ECM starting...");

    // 1. Thread-safe inbox now stores raw bits
    std::mutex m;
    std::condition_variable cv;
    std::queue<std::vector<std::uint8_t>> inbox;

    // 2. New Receiver: Injecting the shared state
    shared::can::Receiver canRx(running, 15000, inbox, m, cv);

    shared::can::Bus canTx(0, 15000);
    canTx.AddPeer("dashboard", 15000);
    canTx.AddPeer("tcm", 15000);

    shared::config::Config config;
    config.LoadFromFile("config.json");
    const auto engineConfig = config.getEngineConfig();
    const auto transmissionConfig = config.getTransmissionConfig();

    ecm::engine::Engine engine(engineConfig, transmissionConfig);
    engine.setSelectedGear(0);
    engine.start();

    // 3. Start Receiver Thread
    std::thread rxThread([&]() {
        canRx.Run();
        });

    std::thread processThread([&]() {
        using clock = std::chrono::steady_clock;
        const auto fastUpdateTick = std::chrono::milliseconds(shared::fastUpdate);
        const auto slowUpdateTick = std::chrono::milliseconds(shared::slowUpdate);

        auto nextSlowTelemetry = clock::now();
        auto nextFastTelemetry = clock::now();

        while (running) {
            std::vector<std::uint8_t> rawData;
            {
                // Wait for work or timeout to handle periodic telemetry
                std::unique_lock<std::mutex> lk(m);
                cv.wait_for(lk, std::chrono::milliseconds(10), [&]() {
                    return !running || !inbox.empty();
                    });

                if (!running) break;
                if (inbox.empty()) goto telemetry; // Skip to telemetry if no message

                rawData = std::move(inbox.front());
                inbox.pop();
            }

            if (!rawData.empty()) {
                shared::bit_parser::BitReader reader(
                    shared::bit_parser::Span<const std::uint8_t>(rawData.data(), rawData.size())
                );

                auto category = static_cast<shared::can::MessageCategory>(reader.readU8());
                auto typeHeader = reader.readU8();

                // ECM cares about CONTROL messages (from Dashboard/TCM)
                if (category == shared::can::MessageCategory::Control) {
                    switch (static_cast<shared::can::headers::Control>(typeHeader)) {
                    case shared::can::headers::Control::ThrottleRequest: {
                        // Re-wrap raw data into the Throttle class to parse value
                        shared::can::message::Throttle msg(std::move(rawData));
                        engine.setThrottle(msg.getValue());
                        break;
                    }
                    default:
                        break;
                    }
                }
                // ECM also tracks CurrentGear from the TCM (which is a Status message)
                else if (category == shared::can::MessageCategory::Status) {
                    if (static_cast<shared::can::headers::Status>(typeHeader) == shared::can::headers::Status::CurrentGear) {
                        shared::can::message::StatusMessage msg(std::move(rawData));
                        engine.setSelectedGear(msg.getValue());
                    }
                }
            }

        telemetry:
            const auto now = clock::now();

            // Fast Telemetry: RPM, Speed, Fuel
            if (now >= nextFastTelemetry) {
                nextFastTelemetry += fastUpdateTick;

                // Using the new message classes + Implicit Conversion in Bus::Send
                canTx.Send(shared::can::message::StatusMessage(
                    shared::can::MessageCategory::Status,
                    shared::can::headers::Status::RPM,
                    static_cast<std::uint32_t>(engine.getRpm())
                ));

                canTx.Send(shared::can::message::StatusMessage(
                    shared::can::MessageCategory::Status,
                    shared::can::headers::Status::Speed,
                    static_cast<std::uint32_t>(engine.getSpeedMph())
                ));

                uint32_t fuel = static_cast<uint32_t>(engine.getFuelPercentage());
                canTx.Send(shared::can::message::StatusMessage(
                    shared::can::MessageCategory::Status,
                    shared::can::headers::Status::Fuel,
                    fuel
                ));

                if (fuel == 0) {
                    canTx.Send(shared::can::message::ErrorMessage(
                        shared::can::MessageCategory::Error,
                        shared::can::headers::Error::NoFuel,
                        "Empty fuel!"
                    ));
                }
            }

            // Slow Telemetry: Engine Temp
            if (now >= nextSlowTelemetry) {
                nextSlowTelemetry += slowUpdateTick;
                canTx.Send(shared::can::message::StatusMessage(
                    shared::can::MessageCategory::Status,
                    shared::can::headers::Status::EngineTemperature,
                    scaleTemp(engine.getCoolantTempC())
                ));
            }
        }
        });

    processThread.join();

    // Shutdown sequence
    running = false;
    engine.stop();
    canRx.Stop();
    cv.notify_all();

    if (rxThread.joinable()) rxThread.join();

    LogFile::Info("All threads stopped cleanly.");
    return 0;
}