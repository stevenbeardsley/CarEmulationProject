#ifndef SHARED_CAN_HEADERS_ERRORTYPE_H
#define SHARED_CAN_HEADERS_ERRORTYPE_H

#include <cstdint>

namespace shared::can::headers
{

    enum class Error : std::uint8_t
    {
        LowFuel = 0,
        NoFuel = 1,
        EngineOverheating = 2,
        EngineStalled = 3,
        ShiftDownRecommended = 4,
        ShiftUpReccommended = 5 // TODO Fix spelling
    };

}

#endif