#ifndef SHARED_CAN_HEADERS_ERRORTYPE_H
#define SHARED_CAN_HEADERS_ERRORTYPE_H

#include <cstdint>

namespace shared::can::headers
{

enum class Error : std::uint8_t
{
    LowFuel = 0,
    NoFuel = 1
    // TODO: Add more
};

}

#endif