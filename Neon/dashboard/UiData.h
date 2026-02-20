#ifndef DASHBOARD_UIDATA_H
#define DASHBOARD_UIDATA_H

#include <cstdint>

namespace dashboard
{

struct UiData
{
    std::uint32_t m_gear;
    std::uint32_t m_speed;
    std::uint32_t m_rpm;
    std::uint32_t m_maxRpms; // TODO: Should be const really  
    std::uint32_t m_engineTemp; 
    std::uint32_t m_fuel;
    bool m_status;
};

}
#endif 