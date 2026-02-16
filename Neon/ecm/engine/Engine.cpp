#include "Engine.h"

#include <algorithm>
#include <chrono>
#include <cmath>

namespace ecm::engine
{
    using clock_t = std::chrono::steady_clock;

    shared::config::Engine Engine::sanitizeEngineCfg(shared::config::Engine cfg)
    {
        if (cfg.idle_rpm < 500)  cfg.idle_rpm = 500;
        if (cfg.max_rpm < 1500) cfg.max_rpm = 1500;
        if (cfg.idle_rpm >= cfg.max_rpm)
            cfg.idle_rpm = std::max(500, cfg.max_rpm - 1000);

        if (cfg.max_torque_nm < 10.0)
            cfg.max_torque_nm = 10.0;

        if (cfg.displacement_l < 0.1)
            cfg.displacement_l = 0.1;

        return cfg;
    }

    shared::config::Transmission Engine::sanitizeTransCfg(shared::config::Transmission cfg)
    {
        if (cfg.m_gears < 0) cfg.m_gears = 0;

        if (cfg.m_finalDrive <= 0.01)
            cfg.m_finalDrive = 0.01;

        // Ensure gearRatios length matches gears if possible
        if (cfg.m_gears > 0 && static_cast<int>(cfg.m_gearRatios.size()) < cfg.m_gears)
        {
            // If config says 5 gears but only gives 4 ratios, clamp gears to provided ratios
            cfg.m_gears = static_cast<int>(cfg.m_gearRatios.size());
        }

        // Defensive: remove non-positive ratios
        for (double& r : cfg.m_gearRatios)
        {
            if (r < 0.01) r = 0.01;
        }

        // reverse can be negative in many configs; we keep it as-is
        if (std::abs(cfg.m_reverseRatio) < 0.01)
            cfg.m_reverseRatio = -3.0;

        return cfg;
    }

    Engine::Engine(const shared::config::Engine& engineCfg,
        const shared::config::Transmission& transCfg)
        : m_engineCfg(sanitizeEngineCfg(engineCfg))
        , m_transCfg(sanitizeTransCfg(transCfg))
        , m_finalDrive(m_transCfg.m_finalDrive)
    {
        // Start in neutral by default; caller should setSelectedGear()
        m_selectedGear = 0;
        m_gearRatio = 0.0;

        // Initialize RPM at idle
        m_rpm = static_cast<double>(m_engineCfg.idle_rpm);
    }

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
        m_cv.notify_all();
    }

    void Engine::setSelectedGear(int gear)
    {
        std::lock_guard<std::mutex> lk(m_mutex);

        m_selectedGear = gear;

        // Neutral
        if (gear == 0)
        {
            m_gearRatio = 0.0;
            m_cv.notify_all();
            return;
        }

        // Reverse
        if (gear == -1)
        {
            m_gearRatio = m_transCfg.m_reverseRatio;
            // For this model we still don't simulate negative vehicle speed;
            // reverse ratio is mainly for displaying RPM linkage if you later add reverse motion.
            m_cv.notify_all();
            return;
        }

        // Forward gears 1..N (prefer ratio vector length)
        const int n = static_cast<int>(m_transCfg.m_gearRatios.size());
        if (gear >= 1 && gear <= n)
        {
            m_gearRatio = m_transCfg.m_gearRatios[gear - 1];
        }
        else
        {
            // Invalid gear -> neutral
            m_selectedGear = 0;
            m_gearRatio = 0.0;
        }

        if (m_gearRatio < 0.01)
            m_gearRatio = 0.0;

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

    double Engine::clampd(double v, double lo, double hi)
    {
        return std::max(lo, std::min(v, hi));
    }

    double Engine::mpsToMph(double mps)
    {
        return mps / 0.44704;
    }

    double Engine::torqueAtRpmNm(double rpm) const
    {
        // Config-driven smooth curve:
        // - peak around ~60% of (redline-idle) above idle
        // - falls toward redline
        // - minimum floor near idle
        const double maxT = std::max(1.0, m_engineCfg.max_torque_nm);

        const double redline = static_cast<double>(m_engineCfg.max_rpm);
        const double idle = static_cast<double>(m_engineCfg.idle_rpm);

        if (redline <= idle + 1.0)
            return maxT;

        const double peakRpm = idle + 0.60 * (redline - idle);
        const double spread = 0.22 * (redline - idle);
        const double x = (rpm - peakRpm) / std::max(1.0, spread);

        double t = maxT * std::exp(-0.5 * x * x);
        t = std::max(t, 0.25 * maxT);
        return t;
    }

    double Engine::computeRedlineSpeedCapMps() const
    {
        // No cap if neutral/invalid ratio
        const double gear = m_gearRatio;
        const double fd = m_finalDrive;

        if (gear <= 0.01 || fd <= 0.01)
            return -1.0;

        const double wheelCirc = 2.0 * 3.141592653589793 * m_wheelRadiusM;
        if (wheelCirc <= 1e-6)
            return -1.0;

        const double totalRatio = gear * fd;
        if (totalRatio <= 1e-6)
            return -1.0;

        const double redlineRpm = static_cast<double>(m_engineCfg.max_rpm);

        const double maxWheelRpm = redlineRpm / totalRatio;
        const double maxWheelRps = maxWheelRpm / 60.0;
        const double maxSpeedMps = maxWheelRps * wheelCirc;
        return maxSpeedMps;
    }

    void Engine::runLoop()
    {
        const auto tickPeriod =
            std::chrono::duration<double>(1.0 / std::max(1.0, m_tickHz));

        auto last = clock_t::now();

        for (;;)
        {
            {
                std::unique_lock<std::mutex> lk(m_mutex);
                m_cv.wait_for(lk, tickPeriod, [&] { return m_stopRequested; });
                if (m_stopRequested)
                    break;
            }

            const auto now = clock_t::now();
            std::chrono::duration<double> dt = now - last;
            last = now;

            const double dtSec = clampd(dt.count(), 0.0, 0.1);
            step(dtSec);
        }
    }

    void Engine::step(double dtSeconds)
    {
        std::lock_guard<std::mutex> lk(m_mutex);

        if (dtSeconds <= 0.0)
            return;

        // Smooth throttle
        const double targetThrottle = static_cast<double>(m_throttlePercent) / 100.0;
        const double alpha = 1.0 - std::exp(-m_throttleResponse * dtSeconds);
        m_effectiveThrottle = m_effectiveThrottle + (targetThrottle - m_effectiveThrottle) * alpha;
        m_effectiveThrottle = clampd(m_effectiveThrottle, 0.0, 1.0);

        // Resistances
        const double resistForce = m_cRolling + (m_cDrag * m_speedMps * m_speedMps);

        // Hard speed cap (locked driveline) per gear at redline
        const double speedCapMps = computeRedlineSpeedCapMps();
        if (speedCapMps > 0.0 && m_speedMps > speedCapMps)
            m_speedMps = speedCapMps;

        // Wheel rpm from speed
        const double wheelCirc = 2.0 * 3.141592653589793 * m_wheelRadiusM;
        const double wheelRps = (wheelCirc > 1e-6) ? (m_speedMps / wheelCirc) : 0.0;
        const double wheelRpm = wheelRps * 60.0;

        const double idleRpm = static_cast<double>(m_engineCfg.idle_rpm);
        const double redlineRpm = static_cast<double>(m_engineCfg.max_rpm);

        // Neutral: let engine free-rev a bit; no drive torque
        if (m_gearRatio <= 0.01)
        {
            const double freeRev = idleRpm + (redlineRpm - idleRpm) * (0.25 + 0.75 * m_effectiveThrottle);
            m_rpm = clampd(freeRev, idleRpm, redlineRpm);

            double netForce = -resistForce;
            m_accelMps2 = netForce / std::max(1.0, m_massKg);

            if (m_speedMps <= 0.01 && m_effectiveThrottle < 0.01)
            {
                m_speedMps = 0.0;
                m_accelMps2 = 0.0;
                return;
            }

            m_speedMps += m_accelMps2 * dtSeconds;
            if (m_speedMps < 0.0) m_speedMps = 0.0;
            return;
        }

        // In-gear: rpm is linked to wheel speed
        const double rpmFromSpeed = wheelRpm * m_gearRatio * m_finalDrive;
        m_rpm = clampd(rpmFromSpeed, idleRpm, redlineRpm);

        // Torque -> wheel force
        const double engTorque = torqueAtRpmNm(m_rpm) * m_effectiveThrottle;
        const double wheelTorque = engTorque * m_gearRatio * m_finalDrive * m_drivetrainEff;
        const double driveForce = (m_wheelRadiusM > 1e-6) ? (wheelTorque / m_wheelRadiusM) : 0.0;

        double netForce = driveForce - resistForce;

        // Hard cap behaviour: at cap don't allow positive net force
        if (speedCapMps > 0.0 && m_speedMps >= speedCapMps - 1e-4)
        {
            if (netForce > 0.0)
                netForce = 0.0;
            m_speedMps = speedCapMps;
        }

        m_accelMps2 = netForce / std::max(1.0, m_massKg);

        // Prevent negative speed (reverse not simulated here)
        if (m_speedMps <= 0.01 && m_effectiveThrottle < 0.01)
        {
            m_speedMps = 0.0;
            m_accelMps2 = 0.0;
            return;
        }

        m_speedMps += m_accelMps2 * dtSeconds;

        if (m_speedMps < 0.0)
            m_speedMps = 0.0;

        // Clamp overshoot due to dt
        if (speedCapMps > 0.0 && m_speedMps > speedCapMps)
            m_speedMps = speedCapMps;
    }
}
