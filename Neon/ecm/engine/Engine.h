#pragma once

#include <cstdint>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

#include "shared/config/Engine.h"
#include "shared/config/Transmission.h"

namespace ecm::engine
{
    class Engine
    {
    public:
        // Engine must be created with config (read from JSON elsewhere)
        Engine(const shared::config::Engine& engineCfg,
            const shared::config::Transmission& transCfg);

        ~Engine();

        Engine(const Engine&) = delete;
        Engine& operator=(const Engine&) = delete;

        // Thread control
        void start();
        void stop();
        bool isRunning() const;

        // Driver inputs
        void setThrottle(std::uint32_t throttlePercent); // 0..100

        // Gear selection:
        //   0 = neutral (no drive)
        //   1..N = forward gears (uses m_gearRatios[gear-1])
        //  -1 = reverse (uses m_reverseRatio)
        void setSelectedGear(int gear);

        // Vehicle parameters (optional overrides)
        void setMassKg(double kg);
        void setWheelRadiusM(double meters);

        // Telemetry
        std::uint32_t getSpeedMph() const;
        std::uint32_t getRpm() const;
        double getAccelerationMps2() const;

    private:
        void runLoop();
        void step(double dtSeconds);

        // Helpers
        static double clampd(double v, double lo, double hi);
        static double mpsToMph(double mps);

        // Torque curve using config
        double torqueAtRpmNm(double rpm) const;

        // Compute max speed at redline for current gear (locked driveline).
        // Returns <= 0 if no cap applies (neutral, invalid ratios, etc.)
        double computeRedlineSpeedCapMps() const;

        // Config validation/sanity
        static shared::config::Engine sanitizeEngineCfg(shared::config::Engine cfg);
        static shared::config::Transmission sanitizeTransCfg(shared::config::Transmission cfg);

    private:
        // Threading
        mutable std::mutex m_mutex;
        std::condition_variable m_cv;
        std::thread m_thread;
        bool m_running{ false };
        bool m_stopRequested{ false };

        // Tick
        double m_tickHz{ 60.0 };

        // Inputs / state
        std::uint32_t m_throttlePercent{ 0 };
        double m_effectiveThrottle{ 0.0 };
        double m_throttleResponse{ 6.0 }; // higher = faster response

        // Config copies (constructed with these)
        shared::config::Engine m_engineCfg{};
        shared::config::Transmission m_transCfg{};

        // Selected gear & active ratio
        int m_selectedGear{ 0 };     // 0 neutral, 1..N, -1 reverse
        double m_gearRatio{ 0.0 };   // active gear ratio (0 = neutral)
        double m_finalDrive{ 1.0 };

        // Vehicle constants / params
        double m_massKg{ 1400.0 };
        double m_wheelRadiusM{ 0.30 };

        // Simple resistances
        double m_cRolling{ 180.0 };  // N
        double m_cDrag{ 0.35 };      // N/(m/s)^2

        // Drivetrain
        double m_drivetrainEff{ 0.90 };

        // Dynamic state
        double m_speedMps{ 0.0 };
        double m_rpm{ 0.0 };
        double m_accelMps2{ 0.0 };
    };
}
