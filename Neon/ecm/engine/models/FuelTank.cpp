#include "FuelTank.h"
#include <algorithm>

namespace ecm::engine::models
{
    void FuelTank::initialize(const double displacementLitres)
    {
        m_displacementL = std::max(0.1, displacementLitres);
        // Scale tank capacity by engine size
        // 1.0L -> 45L tank
        // 2.0L -> 55L tank
        m_capacityL = 40.0 + (m_displacementL * 7.5);
        m_fuelLevelL = m_capacityL; // start full
    }

    void FuelTank::update(const double rpm,
        double throttle01,
        const double idleRpm,
        const double maxRpm,
        const double dtSeconds)
    {
        if (m_fuelLevelL <= 0.0)
            return;

        throttle01 = std::clamp(throttle01, 0.0, 1.0);
        const auto rpmNorm =
            std::clamp((rpm - idleRpm) / std::max(1.0, maxRpm - idleRpm), 0.0, 1.0);

        // Base idle consumption (L/sec)
        auto burnRate = 0.015 * m_displacementL;   
        // Load component
        burnRate += 0.15 * m_displacementL * throttle01;
        // RPM influence
        burnRate += 0.25 * m_displacementL * rpmNorm * throttle01;
        // Rich mixture near redline
        if (rpmNorm > 0.85)
        {
            burnRate *= 1.15;
        }

        m_fuelLevelL -= burnRate * dtSeconds;
        m_fuelLevelL = std::max(m_fuelLevelL, 0.0);
    }

    bool FuelTank::isEmpty() const
    {
        return m_fuelLevelL <= 0.001;
    }

    double FuelTank::getFuelLitres() const
    {
        return m_fuelLevelL;
    }

    double FuelTank::getFuelPercent() const
    {
        if (m_capacityL <= 0.0)
            return 0.0;
        return (m_fuelLevelL / m_capacityL) * 100.0;
    }

    void FuelTank::refuel()
    {
        m_fuelLevelL = m_capacityL;
    }

    void FuelTank::addFuelLitres(const double litres)
    {
        if (litres <= 0.0)
            return;
        m_fuelLevelL += litres;
        m_fuelLevelL = std::min(m_fuelLevelL, m_capacityL);
    }
}