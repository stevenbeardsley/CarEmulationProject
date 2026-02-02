#include "Engine.h"

#include <algorithm>
#include <chrono>
#include <cmath>

namespace ecm::engine
{
    using clock_t = std::chrono::steady_clock;

    double Engine::clampd(double v, double lo, double hi)
    {
        return std::max(lo, std::min(v, hi));
    }

    Engine::Engine() = default;

    Engine::~Engine()
    {
        stop();
    }

    void Engine::start()
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        if (m_running)
            return;

        m_stopRequested = false;
        m_running = true;

        // Launch thread
        m_thread = std::thread(&Engine::runLoop, this);
    }

    void Engine::stop()
    {
        {
            std::lock_guard<std::mutex> lk(m_mutex);
            if (!m_running)
                return;

            m_stopRequested = true;
        }
        m_cv.notify_all();

        if (m_thread.joinable())
            m_thread.join();

        {
            std::lock_guard<std::mutex> lk(m_mutex);
            m_running = false;
            m_stopRequested = false;
        }
    }

    bool Engine::isRunning() const
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        return m_running;
    }

    void Engine::setThrottle(std::uint32_t throttlePercent)
    {
        {
            std::lock_guard<std::mutex> lk(m_mutex);
            m_throttlePercent = (throttlePercent > 100u) ? 100u : throttlePercent;
        }
        // Wake engine thread so it reacts quickly (optional)
        m_cv.notify_all();
    }

    std::uint32_t Engine::getThrottle() const
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        return m_throttlePercent;
    }

    void Engine::setGearRatio(double ratio)
    {
        {
            std::lock_guard<std::mutex> lk(m_mutex);
            m_gearRatio = (ratio > 0.01) ? ratio : 0.01;
        }
        m_cv.notify_all();
    }

    void Engine::setFinalDrive(double ratio)
    {
        {
            std::lock_guard<std::mutex> lk(m_mutex);
            m_finalDrive = (ratio > 0.01) ? ratio : 0.01;
        }
        m_cv.notify_all();
    }

    void Engine::setMassKg(double kg)
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        m_massKg = (kg > 1.0) ? kg : 1.0;
    }

    void Engine::setWheelRadiusM(double meters)
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        m_wheelRadiusM = (meters > 0.05) ? meters : 0.05;
    }

    std::uint32_t Engine::getSpeedMph() const
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        const double mph = mpsToMph(m_speedMps);
        return static_cast<std::uint32_t>(std::round(std::max(0.0, mph)));
    }

    std::uint32_t Engine::getRpm() const
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        return static_cast<std::uint32_t>(std::round(std::max(0.0, m_rpm)));
    }

    double Engine::getAccelerationMps2() const
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        return m_accelMps2;
    }

    double Engine::mpsToMph(double mps)
    {
        return mps / 0.44704;
    }

    double Engine::maxTorqueNm(double rpm) const
    {
        // Simple mid-range torque peak
        const double peakTorque = 260.0; // Nm
        const double peakRpm = 3500.0;
        const double spread = 2200.0;

        const double x = (rpm - peakRpm) / spread;
        double torque = peakTorque * std::exp(-0.5 * x * x);

        // Ensure a minimum torque near idle
        torque = std::max(torque, 80.0);
        return torque;
    }

    void Engine::runLoop()
    {
        const auto tickPeriod =
            std::chrono::duration<double>(1.0 / std::max(1.0, m_tickHz));

        auto last = clock_t::now();

        for (;;)
        {
            // Wait until next tick or stop requested.
            {
                std::unique_lock<std::mutex> lk(m_mutex);
                // Wake on timeout (tick) or stop request.
                m_cv.wait_for(lk, tickPeriod, [&] { return m_stopRequested; });

                if (m_stopRequested)
                    break;
            }

            const auto now = clock_t::now();
            std::chrono::duration<double> dt = now - last;
            last = now;

            // Clamp dt to avoid huge jumps if the container stalls briefly.
            const double dtSec = clampd(dt.count(), 0.0, 0.1);

            // Step simulation
            step(dtSec);
        }
    }

    void Engine::step(double dtSeconds)
    {
        // Copy inputs under lock, update state under same lock.
        // This keeps the model consistent (single mutex protecting everything).
        std::lock_guard<std::mutex> lk(m_mutex);

        if (dtSeconds <= 0.0)
            return;

        // Smooth throttle
        const double targetThrottle = static_cast<double>(m_throttlePercent) / 100.0; // 0..1
        const double alpha = 1.0 - std::exp(-m_throttleResponse * dtSeconds);
        m_effectiveThrottle = m_effectiveThrottle + (targetThrottle - m_effectiveThrottle) * alpha;
        m_effectiveThrottle = clampd(m_effectiveThrottle, 0.0, 1.0);

        // Wheel rpm from speed
        const double wheelCirc = 2.0 * 3.141592653589793 * m_wheelRadiusM;
        const double wheelRps = (wheelCirc > 1e-6) ? (m_speedMps / wheelCirc) : 0.0;
        const double wheelRpm = wheelRps * 60.0;

        // Engine rpm from speed & gearing
        const double rpmFromSpeed = wheelRpm * m_gearRatio * m_finalDrive;
        m_rpm = clampd(rpmFromSpeed, m_idleRpm, m_redlineRpm);

        // Torque -> wheel force
        const double engTorque = maxTorqueNm(m_rpm) * m_effectiveThrottle;
        const double wheelTorque = engTorque * m_gearRatio * m_finalDrive * m_drivetrainEff;
        const double driveForce = (m_wheelRadiusM > 1e-6) ? (wheelTorque / m_wheelRadiusM) : 0.0;

        // Resistances
        const double resistForce = m_cRolling + (m_cDrag * m_speedMps * m_speedMps);

        const double netForce = driveForce - resistForce;
        m_accelMps2 = netForce / std::max(1.0, m_massKg);

        // Prevent negative speed (no reverse in this simple model)
        if (m_speedMps <= 0.01 && m_effectiveThrottle < 0.01)
        {
            m_speedMps = 0.0;
            m_accelMps2 = 0.0;
            return;
        }

        m_speedMps += m_accelMps2 * dtSeconds;

        if (m_speedMps < 0.0)
            m_speedMps = 0.0;
    }
}
