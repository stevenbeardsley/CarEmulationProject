#ifndef SHARED_CAN_MESSAGE_THROTTLE_H
#define SHARED_CAN_MESSAGE_THROTTLE_H

#include "ICanMessage.h"
#include "headers/Control.h"
#include "../MessageCategory.h"
#include <cstdint>
#include <vector>

namespace shared::can::message
{

    class Throttle : public ICanMessage
    {
    public:
        Throttle(MessageCategory category, headers::Control type, std::uint32_t value);
        explicit Throttle(std::vector<std::uint8_t> existingBuffer);

        MessageCategory getCategory() const;
        headers::Control getType() const;
        std::uint32_t getValue() const;

        const std::vector<std::uint8_t>& getRawBuffer() const { return m_buffer; }

    };

}

#endif 