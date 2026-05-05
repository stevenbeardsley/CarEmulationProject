#include "Engine.h"

#include <algorithm>
#include <chrono>
#include <cmath>

namespace ecm::engine
{
    using clock_t = std::chrono::steady_clock;

    shared::config::Engine Engine::sanitizeEngineCfg(shared::config::Engine cfg)
    {
        cfg.idle_rpm = std::max(cfg.idle_rpm, 500);
        cfg.max_rpm = std::max(cfg.max_rpm, 1500);
        if (cfg.idle_rpm >= cfg.max_rpm)
        {
            cfg.idle_rpm = std::max(500, cfg.max_rpm - 1000);
        }

        cfg.max_torque_nm = std::max(cfg.max_torque_nm, 10.0);

        cfg.displacement_l = std::max(cfg.displacement_l, 0.1);
        return cfg;
    }

    shared::config::Transmission Engine::sanitizeTransCfg(shared::config::Transmission cfg)
    {
        cfg.m_gears = std::max(cfg.m_gears, 0);

        cfg.m_finalDrive = std::max(cfg.m_finalDrive, 0.01);

        // Ensure gearRatios length matches gears if possible
        if (cfg.m_gears > 0 && static_cast<int>(cfg.m_gearRatios.size()) < cfg.m_gears)
        {
            // If config says 5 gears but only gives 4 ratios, clamp gears to provided ratios
            cfg.m_gears = static_cast<int>(cfg.m_gearRatios.size());
        }

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
        m_fuel.initialize(m_engineCfg.displacement_l);

        // Initialize RPM at idle
        m_rpm = static_cast<double>(m_engineCfg.idle_rpm);
    }

    Engine::~Engine()
    {
        stop();
    }

    void Engine::start()
    {
        std::scoped_lock lk(m_mutex);
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
            {
                return;
            }

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

    bool Engine::isStalled() const
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        return m_isStalled;
    }

    double Engine::getFuelPercentage() const
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        return m_fuel.getFuelPercent();
    }

    bool Engine::isOutOfFuel() const
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        return m_fuel.isEmpty();
    }

    void Engine::refuel()
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        m_fuel.refuel();
    }

    void Engine::setThrottle(std::uint32_t throttlePercent)
    {
        {
            std::lock_guard<std::mutex> lk(m_mutex);
            m_throttlePercent = (throttlePercent > 100u) ? 100u : throttlePercent;
        }
        m_cv.notify_all();
    }

    void Engine::setBrakeLevel(std::uint32_t brakePercent)
    {
        {
            std::lock_guard<std::mutex> lk(m_mutex);
            m_brakePercent = (brakePercent > 100u) ? 100u : brakePercent;
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

    double Engine::getCoolantTempC() const
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        return m_thermal.getCoolantTempC();
    }

    bool Engine::isOverheating() const
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        return m_thermal.isOverheating();
    }

    double Engine::clampd(double v, double lo, double hi)
    {
        return std::max(lo, std::min(v, hi));
    }

    double Engine::mpsToMph(double mps)
    {
        return mps / 0.44704;
    }

    // Config-driven smooth curve:
    // - peak around ~60% of (redline-idle) above idle
    // - falls toward redline
    // - minimum floor near idle
    double Engine::torqueAtRpmNm(const double rpm) const
    {
        constexpr double kMinTorqueNm = 1.0;
        constexpr double kPeakRpmFraction = 0.60;  // peak torque at 60% of the RPM range
        constexpr double kSpreadFraction = 0.22;  // gaussian width as fraction of RPM range
        constexpr double kMinTorqueFraction = 0.25;  // torque floor as fraction of peak

        const auto maxT = std::max(kMinTorqueNm, m_engineCfg.max_torque_nm);
        const auto redline = m_engineCfg.max_rpm;
        const auto idle = m_engineCfg.idle_rpm;

        if (redline <= idle + 1.0)
            return maxT;

        const auto rpmRange = redline - idle;
        const auto peakRpm = idle + kPeakRpmFraction * rpmRange;
        const auto spread = std::max(1.0, kSpreadFraction * rpmRange);
        const auto x = (rpm - peakRpm) / spread;

        return std::max(maxT * std::exp(-0.5 * x * x), kMinTorqueFraction * maxT);
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

        m_fuel.update(
            m_rpm,
            m_effectiveThrottle,
            static_cast<double>(m_engineCfg.idle_rpm),
            static_cast<double>(m_engineCfg.max_rpm),
            dtSeconds);

        const double idleRpm = static_cast<double>(m_engineCfg.idle_rpm);
        const double redlineRpm = static_cast<double>(m_engineCfg.max_rpm);

        // If RPM exceeds redline by 10%, the engine blows/stalls
        const double overRevThreshold = redlineRpm * 1.10;
        if (m_rpm > overRevThreshold)
        {
            m_isStalled = true;
        }

        bool hasFuel = !m_fuel.isEmpty();
        bool canRun = hasFuel && !m_isStalled;

        // Engine only responds to throttle if it has fuel and isn't blown
        if (!canRun)
        {
            m_effectiveThrottle = 0.0;
        }
        else
        {
            const double targetThrottle = static_cast<double>(m_throttlePercent) / 100.0;
            const double alpha = 1.0 - std::exp(-m_throttleResponse * dtSeconds);
            m_effectiveThrottle = m_effectiveThrottle + (targetThrottle - m_effectiveThrottle) * alpha;
        }

        m_effectiveThrottle = clampd(m_effectiveThrottle, 0.0, 1.0);

        // Process Brake Smoothing (brakes typically respond slightly faster than throttle)
        const double targetBrake = static_cast<double>(m_brakePercent) / 100.0;
        const double brakeAlpha = 1.0 - std::exp(-10.0 * dtSeconds);
        m_effectiveBrake = m_effectiveBrake + (targetBrake - m_effectiveBrake) * brakeAlpha;
        m_effectiveBrake = clampd(m_effectiveBrake, 0.0, 1.0);

        if (m_thermal.isOverheating())
        {
            m_effectiveThrottle *= 0.5;
        }

        const double resistForce = m_cRolling + (m_cDrag * m_speedMps * m_speedMps);
        const double speedCapMps = computeRedlineSpeedCapMps();

        const double wheelCirc = 2.0 * 3.141592653589793 * m_wheelRadiusM;
        const double wheelRps = (wheelCirc > 1e-6) ? (m_speedMps / wheelCirc) : 0.0;
        const double wheelRpm = wheelRps * 60.0;

        // Calculate brake force (Assuming ~1G maximum deceleration: mass * 9.81 m/s^2)
        const double maxBrakeForce = m_massKg * 9.81;
        double currentBrakeForce = maxBrakeForce * m_effectiveBrake;

        // Only apply heavy mechanical braking if the vehicle is actually moving to prevent 
        // massive negative acceleration values from building up while stationary.
        if (m_speedMps < 0.01)
        {
            currentBrakeForce = 0.0;
        }

        // Neutral logic
        if (m_gearRatio <= 0.01)
        {
            const double targetNeutralRpm = canRun ? (idleRpm + (redlineRpm - idleRpm) * (0.25 + 0.75 * m_effectiveThrottle)) : 0.0;
            m_rpm = m_rpm + (targetNeutralRpm - m_rpm) * (1.0 - std::exp(-2.0 * dtSeconds));

            double netForce = -resistForce - currentBrakeForce;
            m_accelMps2 = netForce / std::max(1.0, m_massKg);
        }
        else
        {

            // In-gear: rpm is linked to wheel speed, but smoothed to avoid instant snap
            const double rpmFromSpeed = wheelRpm * m_gearRatio * m_finalDrive;
            const double floorRpm = canRun ? idleRpm : 0.0;
            const double targetRpm = std::max(floorRpm, rpmFromSpeed);

            // Tune kRpmSlewRate to taste (e.g. 800–1500 RPM/s feels natural)
            constexpr double kRpmSlewRate = 1800.0;
            const double maxDelta = kRpmSlewRate * dtSeconds;
            m_rpm += clampd(targetRpm - m_rpm, -maxDelta, maxDelta);

            // Cap the RPM used for torque calculation so your curve doesn't break
            const double generatingRpm = std::min(m_rpm, redlineRpm);
            const double engTorque = torqueAtRpmNm(generatingRpm) * m_effectiveThrottle;
            const double wheelTorque = engTorque * m_gearRatio * m_finalDrive * m_drivetrainEff;
            const double driveForce = (m_wheelRadiusM > 1e-6) ? (wheelTorque / m_wheelRadiusM) : 0.0;

            double netForce = driveForce - resistForce - currentBrakeForce;

            // Engine Braking & Over-Rev Handling
            if (speedCapMps > 0.0 && m_speedMps > speedCapMps)
            {
                const double overSpeedMps = m_speedMps - speedCapMps;
                netForce -= (overSpeedMps * 500.0);

                netForce = std::min(netForce, 0.0);
            }
            else if (rpmFromSpeed > idleRpm && m_effectiveThrottle < 0.15)
            {
                // Ramps from 0 (at 15% throttle) to full (at 0% throttle)
                const double liftFraction = 1.0 - (m_effectiveThrottle / 0.15);
                const double engineBrakeFactor = liftFraction * 0.03;
                netForce -= (rpmFromSpeed * engineBrakeFactor);
            }
            // Severe mechanical drag if the engine is completely dead/blown
            if (!canRun) {
                netForce -= 250.0; // Apply a force, emulating a heavy mechanical brake
            }

            m_accelMps2 = netForce / std::max(1.0, m_massKg);
        }

        m_speedMps += m_accelMps2 * dtSeconds;
        m_speedMps = std::max(m_speedMps, 0.0);

        m_thermal.update(m_rpm, m_effectiveThrottle, idleRpm, redlineRpm, dtSeconds);
    }
}