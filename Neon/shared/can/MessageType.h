#ifndef SHARED_CAN_MESSAGE_TYPE_H
#define SHARED_CAN_MESSAGE_TYPE_H

#include <string>

namespace shared::can
{

enum class MessageType
{
	Speed, 
	Gear,
	RPM
};

inline const std::string toString(MessageType type)
{
    switch (type)
    {
    case MessageType::Gear:    
        return "gear";
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