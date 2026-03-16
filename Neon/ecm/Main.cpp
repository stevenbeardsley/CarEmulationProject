#include "LogFile.h"
#include "Process.h"
#include "shared/can/Receiver.h"
#include "shared/can/Bus.h"
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

void static signalHandler(int) {
    LogFile::info("Received stop signal, shutting down...");
    running = false;
}

static inline std::int32_t scaleTemp(double tempC) {
    return static_cast<std::int32_t>(std::llround(tempC * 10.0));
}

int main() {
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    LogFile::instance().setLogFile("ecm.log");
    LogFile::instance().setLevel(LogLevel::DEBUG);
    LogFile::info("ECM starting...");

    std::mutex m;
    std::condition_variable cv;
    std::queue<std::vector<std::uint8_t>> inbox;

    shared::can::Receiver canRx(running, 15000, inbox, m, cv);

    shared::can::Bus canTx(0, 15000);
    canTx.addPeer("dashboard", 15000);
    canTx.addPeer("tcm", 15000);

    shared::config::Config config;
    config.LoadFromFile("config.json");
    const auto& engineConfig = config.getEngineConfig();
    const auto& transmissionConfig = config.getTransmissionConfig();

    ecm::engine::Engine engine(engineConfig, transmissionConfig);
    engine.setSelectedGear(0);
    engine.start();

    std::thread rxThread([&]() {
        canRx.Run();
        });

    std::thread processThread([&]() {
        using clock = std::chrono::steady_clock;
        const auto fastUpdateTick = std::chrono::milliseconds(shared::fastUpdate);
        const auto slowUpdateTick = std::chrono::milliseconds(shared::slowUpdate);

        auto nextSlowTelemetry = clock::now();
        auto nextFastTelemetry = clock::now();

        // Track gear locally so we don't recommend shifting in Neutral/Reverse
        int currentGear = 0;

        while (running) 
        {
            std::vector<std::uint8_t> rawData;
            {
                std::unique_lock<std::mutex> lk(m);
                cv.wait_for(lk, std::chrono::milliseconds(10), [&]() 
                    {
                        return !running || !inbox.empty();
                    });

                if (!running) break;
                if (inbox.empty()) goto telemetry;

                rawData = std::move(inbox.front());
                inbox.pop();
            }

            if (!rawData.empty()) 
            {
                shared::bit_parser::BitReader reader(
                    shared::bit_parser::Span<const std::uint8_t>(rawData.data(), rawData.size())
                );

                auto category = static_cast<shared::can::MessageCategory>(reader.readU8());
                auto typeHeader = reader.readU8();

                if (category == shared::can::MessageCategory::Control) 
                {
                    switch (static_cast<shared::can::headers::Control>(typeHeader))
                    {
                    case shared::can::headers::Control::ThrottleRequest: 
                    {
                        shared::can::message::Throttle msg(std::move(rawData));
                        engine.setThrottle(msg.getValue());
                        break;
                    }
                    default:
                        break;
                    }
                }
                else if (category == shared::can::MessageCategory::Status) 
                {
                    if (static_cast<shared::can::headers::Status>(typeHeader) == shared::can::headers::Status::CurrentGear)
                    {
                        shared::can::message::StatusMessage msg(std::move(rawData));
                        currentGear = static_cast<int>(msg.getValue()); // Update local tracker
                        engine.setSelectedGear(currentGear);
                    }
                }
            }

        telemetry:
            const auto now = clock::now();

            // Fast Telemetry: RPM, Speed, Fuel
            if (now >= nextFastTelemetry)
            {
                nextFastTelemetry += fastUpdateTick;

                std::uint32_t currentRpm = engine.getRpm();
                std::uint32_t currentSpeed = engine.getSpeedMph();

                canTx.send(shared::can::message::StatusMessage(
                    shared::can::MessageCategory::Status,
                    shared::can::headers::Status::RPM,
                    currentRpm
                ));

                canTx.send(shared::can::message::StatusMessage(
                    shared::can::MessageCategory::Status,
                    shared::can::headers::Status::Speed,
                    currentSpeed
                ));

                uint32_t fuel = static_cast<uint32_t>(engine.getFuelPercentage());
                canTx.send(shared::can::message::StatusMessage(
                    shared::can::MessageCategory::Status,
                    shared::can::headers::Status::Fuel,
                    fuel
                ));

                if (fuel <= 25 && fuel != 0)
                {
                    canTx.send(shared::can::message::ErrorMessage(
                        shared::can::MessageCategory::Error,
                        shared::can::headers::Error::LowFuel,
                        "Warning: Low fuel."
                    ));
                }
                
                if (fuel == 0)
                {
                    canTx.send(shared::can::message::ErrorMessage(
                        shared::can::MessageCategory::Error,
                        shared::can::headers::Error::NoFuel,
                        "Critical: Empty fuel."
                    ));
                }

                if (engine.isStalled())
                {
                    canTx.send(shared::can::message::ErrorMessage(
                        shared::can::MessageCategory::Error,
                        shared::can::headers::Error::EngineStalled,
                        "Critical: Engine has stalled due to over-rev."
                    ));
                }

                // Only recommend shifting if in a forward driving gear
                if (currentGear > 0 && !engine.isStalled())
                {
                    if (currentRpm > static_cast<std::uint32_t>(engineConfig.max_rpm * 0.85))
                    {
                        canTx.send(shared::can::message::ErrorMessage(
                            shared::can::MessageCategory::Error,
                            shared::can::headers::Error::ShiftUpReccommended,
                            "Warning: Shift up recommended."
                        ));
                    }
                    else if (currentGear > 1 && currentSpeed > 5 && currentRpm < static_cast<std::uint32_t>(engineConfig.idle_rpm * 1.25))
                    {
                        canTx.send(shared::can::message::ErrorMessage(
                            shared::can::MessageCategory::Error,
                            shared::can::headers::Error::ShiftDownRecommended,
                            "Warning: Shift down recommended."
                        ));
                    }
                }
            }

            // Slow Telemetry: Engine Temp
            if (now >= nextSlowTelemetry) {
                nextSlowTelemetry += slowUpdateTick;
                const auto temperature = scaleTemp(engine.getCoolantTempC());
                canTx.send(shared::can::message::StatusMessage(
                    shared::can::MessageCategory::Status,
                    shared::can::headers::Status::EngineTemperature,
                    temperature));

                if (temperature >= 1000)
                {
                    canTx.send(shared::can::message::ErrorMessage(
                        shared::can::MessageCategory::Error,
                        shared::can::headers::Error::EngineOverheating,
                        "Engine overheating"
                    ));
                }
            }
        }
        });

    processThread.join();

    running = false;
    engine.stop();
    canRx.Stop();
    cv.notify_all();

    if (rxThread.joinable()) rxThread.join();

    LogFile::info("All threads stopped cleanly.");
    return 0;
}