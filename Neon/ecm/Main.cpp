#include "LogFile.h"
#include "Process.h"
#include "shared/can/Receiver.h"
#include "shared/can/Bus.h"
#include "shared/can/Message.h"
#include "shared/can/MessageType.h"

#include "ecm/engine/Engine.h"

// Config structs
#include "shared/config/Engine.h"
#include "shared/config/Transmission.h"
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

void signalHandler(int)
{
    LogFile::Info("Received stop signal, shutting down...");
    running = false;
}

static inline std::int32_t scaleAccel(double accelMps2)
{
    // m/s^2 -> milli-(m/s^2)
    return static_cast<std::int32_t>(std::llround(accelMps2 * 1000.0));
}

int main()
{
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    LogFile::Instance().setLogFile("ecm.log");
    LogFile::Instance().setLevel(LogLevel::DEBUG);
    LogFile::Info("ECM starting...");

    // Thread-safe inbox from CAN RX -> processing loop
    std::mutex m;
    std::condition_variable cv;
    std::queue<shared::can::Message> inbox;

    // CAN receiver (UDP)
    shared::can::Receiver canRx(running, /*listenPort=*/15000);

    // CAN transmitter (UDP broadcast)
    shared::can::Bus canTx(
        0,      // ephemeral bind
        15000   // default peer destination
    );

    if (!canTx.AddPeer("dashboard", 15000))
        LogFile::Error("Error: dashboard was unable to add as a peer.");

    if (!canTx.AddPeer("tcm", 15000))
        LogFile::Error("Error: tcm was unable to add as a peer.");

    // ---------------------------------------------------------------------
    // CONFIG: Replace this section with your actual JSON config loader.
    // ---------------------------------------------------------------------
    shared::config::Config config;
    config.LoadFromFile("config.json");

    const auto engineConfig = config.getEngineConfig();
    const auto transmissionConfig = config.getTransmissionConfig();
    // ---------------------------------------------------------------------

    ecm::engine::Engine engine(engineConfig, transmissionConfig);

    // Optional: start in 1st gear or neutral.
    // If TCM will always send CurrentGear soon, neutral is fine.
    engine.setSelectedGear(0);
    engine.start();

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

    // ECM control loop (process commands + periodic telemetry)
    std::thread processThread([&]() {
        using clock = std::chrono::steady_clock;

        // Telemetry rates
        constexpr auto tickPeriod = std::chrono::milliseconds(20);      // 50 Hz loop
        constexpr auto telemetryPeriod = std::chrono::milliseconds(50); // 20 Hz telemetry broadcast
        auto nextTick = clock::now();
        auto nextTelemetry = clock::now();

        while (running)
        {
            nextTick += tickPeriod;

            {
                std::unique_lock<std::mutex> lk(m);
                cv.wait_until(lk, nextTick, [&]() { return !running || !inbox.empty(); });
            }

            if (!running)
                break;

            // Drain inbox quickly (don’t hold lock while acting)
            for (;;)
            {
                shared::can::Message msg(shared::can::MessageType::RPM, 0);
                {
                    std::lock_guard<std::mutex> lk(m);
                    if (inbox.empty()) break;
                    msg = std::move(inbox.front());
                    inbox.pop();
                }

                switch (msg.getMessageType())
                {
                case shared::can::MessageType::ThrottleRequest:
                {
                    LogFile::Info("Throttle Request received.");
                    const auto thr = msg.getValue(); // expected 0..100
                    engine.setThrottle(static_cast<std::uint32_t>(thr));
                    break;
                }

                case shared::can::MessageType::CurrentGear:
                {
                    // NEW: Use gear number directly (ratios come from transCfg)
                    // Expecting: 0 neutral, 1..N forward, -1 reverse (if you use it)
                    const int gear = msg.getValue();

                    engine.setSelectedGear(gear);

                    LogFile::Debug("ECM: CurrentGear=" + std::to_string(gear));
                    break;
                }

                case shared::can::MessageType::GearDownRequest:
                case shared::can::MessageType::GearUpRequest:
                    LogFile::Info("Message received that the ECM is not subscribed to.");
                    break;

                default:
                    LogFile::Info("Unaccounted for message type received.");
                    break;
                }
            }

            // Periodic telemetry broadcast
            const auto now = clock::now();
            if (now >= nextTelemetry)
            {
                nextTelemetry = now + telemetryPeriod;

                const auto rpm = engine.getRpm();
                const auto speed = engine.getSpeedMph();
                const auto accel = scaleAccel(engine.getAccelerationMps2());

                try
                {
                    canTx.Send(shared::can::Message(shared::can::MessageType::RPM, (int)rpm));
                    canTx.Send(shared::can::Message(shared::can::MessageType::Speed, (int)speed));
                    // canTx.Send(shared::can::Message(shared::can::MessageType::Acceleration, accel));

                    LogFile::Info(
                        "ECM Telemetry | "
                        "RPM=" + std::to_string(rpm) +
                        " | Speed=" + std::to_string(speed) + " mph"
                        " | Accel=" + std::to_string(accel) + " (m/s^2 * 1000)"
                    );
                }
                catch (...)
                {
                    LogFile::Warn("ECM: telemetry send failed");
                }
            }
        }

        LogFile::Info("ECM process thread exiting...");
        });

    // Main thread blocks; container stays alive until processThread exits
    processThread.join();

    // Shutdown order
    running = false;
    engine.stop();

    canRx.Stop();
    cv.notify_all();

    if (rxThread.joinable())
        rxThread.join();

    LogFile::Info("All threads stopped cleanly.");
    return 0;
}
