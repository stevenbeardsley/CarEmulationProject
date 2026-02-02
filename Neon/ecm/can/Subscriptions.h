#ifndef ECM_CAN_SUBSCRIPTIONS_H
#define ECM_CAN_SUBSCRIPTIONS_H

#include "shared/can/MessageType.h"
#include <array>

namespace ecm::can
{
// TODO: remove this(?)

constexpr std::array<::shared::can::MessageType, 3> subscriptions = {
    ::shared::can::MessageType::CurrentGear,
    ::shared::can::MessageType::Speed,
    ::shared::can::MessageType::RPM
};

// TODO: implement isSubscribed function 
}
#endif 