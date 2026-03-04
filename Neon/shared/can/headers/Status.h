#ifndef SHARED_CAN_HEADERS_STATUS_H
#define SHARED_CAN_HEADERS_STATUS_H

#include <string>

namespace shared::can::headers
{

enum class Status
{
    CurrentGear,
    Acceleration,
    EngineTemperature,
    Fuel,
    RPM,
    EmptyFuel,
    Speed
};

}

#endif 