#ifndef SHARED_CAN_MESSAGE_THROTTLE_H
#define SHARED_CAN_MESSAGE_THROTTLE_H

#include "ICanMessage.h"
#include "headers/Control.h"
#include "../MessageCategory.h"
#include <cstdint>
#include <vector>

namespace shared::can::message
{

    class SpeedControl : public ICanMessage
    {
    public:
        SpeedControl(MessageCategory category, headers::Control type, std::uint32_t value);
        explicit SpeedControl(std::vector<std::uint8_t> existingBuffer);

        [[nodiscard]] MessageCategory getCategory() const;
        [[nodiscard]] headers::Control getType() const;
        [[nodiscard]] std::uint32_t getValue() const;

        [[nodiscard]] const std::vector<std::uint8_t>& getRawBuffer() const { return m_buffer; }

    };

}

#endif 