#pragma once

#include <cstdint>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

#include "shared/config/Engine.h"
#include "shared/config/Transmission.h"
#include "models/FuelTank.h"
#include "models/ThermalModel.h"

namespace ecm::engine
{
    class Engine
    {
    public:
        Engine(const shared::config::Engine& engineCfg,
            const shared::config::Transmission& transCfg);

        ~Engine();

        Engine(const Engine&) = delete;
        Engine& operator=(const Engine&) = delete;

        void start();
        void stop();
        bool isRunning() const;

        void setThrottle(std::uint32_t throttlePercent);
        void setSelectedGear(int gear);

        void setMassKg(double kg);
        void setWheelRadiusM(double meters);

        [[nodiscard]] std::uint32_t getSpeedMph() const;
        [[nodiscard]] std::uint32_t getRpm() const;
        [[nodiscard]] double getAccelerationMps2() const;

        // Temperature telemetry (delegated)
        [[nodiscard]] double getCoolantTempC() const;
        [[nodiscard]] bool isOverheating() const;

        [[nodiscard]]
        bool isStalled() const;

        // Fuel system 
        [[nodiscard]] double getFuelPercentage() const;
        [[nodiscard]] bool isOutOfFuel() const; // TODO: Use 

    private:
        void runLoop();
        void step(double dtSeconds);

        static double clampd(double v, double lo, double hi);
        static double mpsToMph(double mps);

        double torqueAtRpmNm(double rpm) const;
        double computeRedlineSpeedCapMps() const;

        static shared::config::Engine sanitizeEngineCfg(shared::config::Engine cfg);
        static shared::config::Transmission sanitizeTransCfg(shared::config::Transmission cfg);

    private:
        mutable std::mutex m_mutex;
        std::condition_variable m_cv;
        std::thread m_thread;
        bool m_running{ false };
        bool m_stopRequested{ false };
        bool m_isStalled{ false };

        double m_tickHz{ 60.0 };

        std::uint32_t m_throttlePercent{ 0 };
        double m_effectiveThrottle{ 0.0 };
        double m_throttleResponse{ 6.0 };

        shared::config::Engine m_engineCfg{};
        shared::config::Transmission m_transCfg{};

        int m_selectedGear{ 0 };
        double m_gearRatio{ 0.0 };
        double m_finalDrive{ 1.0 };

        double m_massKg{ 1400.0 };
        double m_wheelRadiusM{ 0.30 };

        double m_cRolling{ 180.0 };
        double m_cDrag{ 0.35 };
        double m_drivetrainEff{ 0.90 };

        double m_speedMps{ 0.0 };
        double m_rpm{ 0.0 };
        double m_accelMps2{ 0.0 };

        // Extracted subsystem
        models::ThermalModel m_thermal;
        models::FuelTank m_fuel;
    };
}
