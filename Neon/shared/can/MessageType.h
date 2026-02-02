#ifndef SHARED_CAN_MESSAGE_TYPE_H
#define SHARED_CAN_MESSAGE_TYPE_H

#include <string>

namespace shared::can
{

enum class MessageType
{
	GearUpRequest,
    GearDownRequest,
    CurrentGear,
    ThrottleRequest,
    Acceleration,
	RPM, // TODO: Implement
    Speed // TODO: Implement
};

inline const std::string toString(MessageType type)
{
    switch (type)
    {
    case MessageType::GearUpRequest:    
        return "gear up";
    case MessageType::Speed:   
        return "speed";
    case MessageType::RPM:     
        return "rpm";
    default:
        return "unknown";
    }
}

}

#endif 