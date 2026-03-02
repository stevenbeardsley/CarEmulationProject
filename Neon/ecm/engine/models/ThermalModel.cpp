#include "ThermalModel.h"
#include <algorithm>
#include <cmath>

namespace ecm::engine::models
{
    // Initialize the engine at ambient temperature, not overheating
    ThermalModel::ThermalModel(double ambientC, double heatRate, double coolRate, double overheatC)
        : m_ambientC(ambientC),
        m_heatRate(heatRate),
        m_coolRate(coolRate),
        m_overheatC(overheatC),
        m_tempC(ambientC),
        m_overheating(false)
    {
    }

    double ThermalModel::clampd(double v, double lo, double hi)
    {
        return std::max(lo, std::min(v, hi));
    }

    void ThermalModel::update(double rpm,
        double throttle,
        double idleRpm,
        double redlineRpm,
        double dtSeconds)
    {
        if (dtSeconds <= 0.0)
            return;

        const auto rpmLoad =
            clampd((rpm - idleRpm) /
                std::max(1.0, redlineRpm - idleRpm),
                0.0, 1.0);

        // m_heatRate is now realistically scaled (e.g., max 4.5 C/sec)
        const auto heatInput =
            m_heatRate * rpmLoad * throttle;

        // 1. Calculate heat gain normally
        m_tempC += heatInput * dtSeconds;

        // 2. Apply cooling using an exponential decay formula
        // This ensures cooling never pushes temp below ambient
        double coolingFactor = std::exp(-m_coolRate * dtSeconds);
        m_tempC = m_ambientC + (m_tempC - m_ambientC) * coolingFactor;

        if (m_tempC < m_ambientC)
        {
            m_tempC = m_ambientC;
        }

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