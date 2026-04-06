#ifndef SHARED_CAN_MESSAGE_GEARCHANGE_H
#define SHARED_CAN_MESSAGE_GEARCHANGE_H

#include "ICanMessage.h"
#include "headers/Control.h"
#include "../MessageCategory.h"
#include <cstdint>
#include <vector>

namespace shared::can::message
{

    class CommandMessage : public ICanMessage
    {
    public:
        CommandMessage(MessageCategory category, headers::Control type);
        explicit CommandMessage(std::vector<std::uint8_t> existingBuffer);

        [[nodiscard]] MessageCategory getCategory() const;
        [[nodiscard]] headers::Control getType() const;

    };

}

#endif 