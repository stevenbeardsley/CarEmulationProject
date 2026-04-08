#ifndef DASHBOARD_UIDATA_H
#define DASHBOARD_UIDATA_H

#include <cstdint>
#include <string>
#include <vector>

#include "shared/can/headers/Error.h"

namespace dashboard
{

struct ErrorMessage
{
    shared::can::headers::Error m_code;
    std::string m_message;
};

struct UiData
{
    std::uint32_t m_gear;
    std::uint32_t m_speed;
    std::uint32_t m_rpm;
    std::uint32_t m_maxRpms; 
    std::uint32_t m_engineTemp; 
    std::uint32_t m_fuel;
    std::vector<ErrorMessage> m_activeErrors;
    bool m_status = true;
};

}
#endif 