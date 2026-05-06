#include "ThermalModel.h"
#include <algorithm>
#include <cmath>

namespace ecm::engine::models
{
    // Initialize the engine at ambient temperature
    ThermalModel::ThermalModel(const double ambientC, const double heatRate, const double coolRate, double overheatC)
        : m_ambientC(ambientC),
        m_heatRate(heatRate),
        m_coolRate(coolRate),
        m_overheatC(overheatC),
        m_tempC(ambientC),
        m_overheating(false)
    {
    }

    double ThermalModel::clampd(const double v, const double lo, const double hi)
    {
        return std::max(lo, std::min(v, hi));
    }

    void ThermalModel::update(const double rpm,
        const double throttle,
        const double idleRpm,
        const double redlineRpm,
        const double dtSeconds)
    {
        if (dtSeconds <= 0.0)
            return;

        const auto rpmLoad =
            clampd((rpm - idleRpm) /
                std::max(1.0, redlineRpm - idleRpm),
                0.0, 1.0);

        // Take 30% of base heat rate and apply it purely based on RPM.
        // Squaring the rpmLoad (rpmLoad * rpmLoad) means heat spikes dramatically 
        // as the engine approaches redline, even if the throttle is completely closed.
        const auto frictionHeat = (m_heatRate * 0.30) * (rpmLoad * rpmLoad);

        // Require both an open throttle and engine speed to generate heat.
        const auto combustionHeat = (m_heatRate * 0.70) * rpmLoad * throttle;

        const auto heatInput = frictionHeat + combustionHeat;

        m_tempC += heatInput * dtSeconds;

        double coolingFactor = std::exp(-m_coolRate * dtSeconds);
        m_tempC = m_ambientC + (m_tempC - m_ambientC) * coolingFactor;

        m_tempC = std::max(m_tempC, m_ambientC);

        if (m_tempC >= m_overheatC)
        {
            m_overheating = true;
        }
        else if (m_tempC < m_overheatC - 5.0)
        {
            m_overheating = false;
        }
    }

    double ThermalModel::getCoolantTempC() const
    {
        return m_tempC;
    }

    bool ThermalModel::isOverheating() const
    {
        return m_overheating;
    }
}
