#ifndef DASHBOARD_UIDATA_H
#define DASHBOARD_UIDATA_H

#include <cstdint>

namespace dashboard
{

struct UiData
{
    std::uint32_t m_gear;
    std::uint32_t m_speed;
    bool m_status;
};

}
#endif 