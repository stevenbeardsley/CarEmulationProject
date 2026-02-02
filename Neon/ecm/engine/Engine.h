#ifndef ECM_ENGINE_H
#define ECM_ENGINE_H

#include <cstdint>
#include <mutex>
#include <condition_variable>
#include <thread>

namespace ecm::engine
{
    class Engine
    {
    public:
        Engine();
        ~Engine();

        Engine(const Engine&) = delete;
        Engine& operator=(const Engine&) = delete;

        // Starts/stops the internal engine simulation thread.
        void start();
        void stop();
        bool isRunning() const;

        // Driver input (0..100)
        void setThrottle(std::uint32_t throttlePercent);
        std::uint32_t getThrottle() const;

        // Optional drivetrain knobs (set by TCM or fixed)
        void setGearRatio(double ratio);
        void setFinalDrive(double ratio);
        void setMassKg(double kg);
        void setWheelRadiusM(double meters);

        // Telemetry (safe to call from any thread)
        std::uint32_t getSpeedMph() const;
        std::uint32_t getRpm() const;
        double getAccelerationMps2() const;

    private:
        void runLoop();
        void step(double dtSeconds);

        // Helpers
        static double mpsToMph(double mps);
        double maxTorqueNm(double rpm) const;
        static double clampd(double v, double lo, double hi);

    private:
        // Threading
        mutable std::mutex m_mutex;
        std::condition_variable m_cv;
        std::thread m_thread;
        bool m_running{ false };
        bool m_stopRequested{ false };

        // --- Inputs (shared) ---
        std::uint32_t m_throttlePercent{ 0 }; // 0..100
        double m_gearRatio{ 3.50 }; // How many times the engine rotates for one rotation of the wheel
        double m_finalDrive{ 3.90 };

        // --- State (shared) ---
        double m_speedMps{ 0.0 };
        double m_accelMps2{ 0.0 };
        double m_rpm{ 900.0 };
        double m_effectiveThrottle{ 0.0 }; // smoothed 0..1

        // --- Parameters (shared) ---
        double m_massKg{ 1400.0 };
        double m_wheelRadiusM{ 0.32 };
        double m_drivetrainEff{ 0.85 };

        double m_idleRpm{ 900.0 };
        double m_redlineRpm{ 6500.0 };

        double m_cRolling{ 160.0 };
        double m_cDrag{ 0.42 };

        double m_throttleResponse{ 6.0 }; // 1/sec (higher = snappier)

        // Tick period for internal loop
        double m_tickHz{ 50.0 }; // default 50Hz
    };
}

#endif
