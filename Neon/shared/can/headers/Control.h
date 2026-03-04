#ifndef SHARED_CAN_HEADERS_CONTROL_H
#define SHARED_CAN_HEADERS_CONTROL_H

#include <string>

namespace shared::can::headers
{

enum class Control
{
    GearUpRequest,
    GearDownRequest,
    ThrottleRequest,
};

}

#endif 