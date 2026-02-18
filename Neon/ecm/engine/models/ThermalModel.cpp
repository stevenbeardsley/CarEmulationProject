#include "ThermalModel.h"
#include <algorithm>
#include <cmath>

namespace ecm::engine::models
{
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

        const auto heatInput =
            m_heatRate * rpmLoad * throttle;

        const auto cooling =
            m_coolRate * (m_tempC - m_ambientC);

        m_tempC += (heatInput - cooling) * dtSeconds;

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
