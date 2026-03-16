#ifndef SHARED_CAN_MESSAGE_GEARCHANGE_H
#define SHARED_CAN_MESSAGE_GEARCHANGE_H

#include "ICanMessage.h"
#include "headers/Control.h"
#include "../MessageCategory.h"
#include <cstdint>
#include <vector>

namespace shared::can::message
{

    class GearChange : public ICanMessage
    {
    public:
        GearChange(MessageCategory category, headers::Control type);
        explicit GearChange(std::vector<std::uint8_t> existingBuffer);

        [[nodiscard]] MessageCategory getCategory() const;
        [[nodiscard]] headers::Control getType() const;

        [[nodiscard]] const std::vector<std::uint8_t>& getRawBuffer() const { return m_buffer; }

    };

}

#endif 