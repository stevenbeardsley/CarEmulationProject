#include "LogFile.h"
#include "Process.h"
#include "shared/can/Receiver.h"
#include "shared/can/Bus.h"
#include "shared/can/Message.h"
#include "shared/can/MessageType.h"

#include "ecm/engine/Engine.h"   

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
    shared::can::Bus canTx(/*defaultPort=*/15000);
    canTx.AddPeer("dashboard");
    canTx.AddPeer("tcm");

    //Start Engine
    ecm::engine::Engine engine;
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
        constexpr auto tickPeriod = std::chrono::milliseconds(20);  // 50 Hz loop
        constexpr auto telemetryPeriod = std::chrono::milliseconds(50); // 20 Hz telemetry broadcast
        auto nextTick = clock::now();
        auto nextTelemetry = clock::now();

        while (running)
        {
            // Wait until either messages arrive OR next tick time
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
                    // Value expected 0..100
                    auto thr = static_cast<std::uint32_t>(msg.getValue());
                    engine.setThrottle(thr);
                    LogFile::Debug("ECM: ThrottleRequest=" + std::to_string(thr));
                    break;
                }

                case shared::can::MessageType::CurrentGear:
                {
                    // If your TCM sends gear number, you can map to ratios here.
                    // Example mapping (tune later):
                    // 1->3.50, 2->2.10, 3->1.40, 4->1.00, 5->0.83
                    const int gear = msg.getValue();
                    double ratio = 1.0;
                    switch (gear)
                    {
                    case 1: ratio = 3.50; break;
                    case 2: ratio = 2.10; break;
                    case 3: ratio = 1.40; break;
                    case 4: ratio = 1.00; break;
                    case 5: ratio = 0.83; break;
                    default: ratio = 1.00; break;
                    }
                    engine.setGearRatio(ratio);
                    LogFile::Debug("ECM: CurrentGear=" + std::to_string(gear) +
                        " ratio=" + std::to_string(ratio));
                    break;
                }

                // If you add these types:
                // case shared::can::MessageType::GearRatio:
                // {
                //     // e.g. send ratio*1000 as int32
                //     const double ratio = msg.getValue() / 1000.0;
                //     engine.setGearRatio(ratio);
                //     break;
                // }

                // case shared::can::MessageType::TelemetryRequest:
                // {
                //     // immediate one-shot telemetry
                //     const auto rpm = engine.getRpm();
                //     const auto spd = engine.getSpeedMph();
                //     const auto acc = scaleAccel(engine.getAccelerationMps2());
                //     canTx.Send(shared::can::Message(shared::can::MessageType::RPM, (int)rpm));
                //     canTx.Send(shared::can::Message(shared::can::MessageType::Speed, (int)spd));
                //     canTx.Send(shared::can::Message(shared::can::MessageType::Acceleration, acc));
                //     break;
                // }

                default:
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

                // These require MessageType::Speed and MessageType::Acceleration to exist.
                // If you don't have them yet, add them, or temporarily re-use existing types.
                try {
                    canTx.Send(shared::can::Message(shared::can::MessageType::RPM, (int)rpm));
                    canTx.Send(shared::can::Message(shared::can::MessageType::Speed, (int)speed));
                    //canTx.Send(shared::can::Message(shared::can::MessageType::Acceleration, accel)); // Acceleration not yet cared about

                    LogFile::Info(
                        "ECM Telemetry | "
                        "RPM=" + std::to_string(rpm) +
                        " | Speed=" + std::to_string(speed) + " mph"
                        " | Accel=" + std::to_string(accel) + " (m/s^2 * 1000)"
                    );

                    // Optional heartbeat:
                    // canTx.Send(shared::can::Message(shared::can::MessageType::Heartbeat, 1));
                }
                catch (...)
                {
                    // If your Bus can throw, log it. Keeping it simple here.
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
