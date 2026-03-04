#ifndef SHARED_CAN_MESSAGE_STATUSMESSAGE_H
#define SHARED_CAN_MESSAGE_STATUSMESSAGE_H

#include "ICanMessage.h"
#include "headers/Status.h"
#include "../MessageCategory.h"
#include <cstdint>
#include <vector>

namespace shared::can::message
{

    class StatusMessage : public ICanMessage
    {
    public:
        StatusMessage(MessageCategory category, headers::Status type, std::uint32_t value);
        explicit StatusMessage(std::vector<std::uint8_t> existingBuffer);

        MessageCategory getCategory() const;
        headers::Status getType() const;
        std::uint32_t getValue() const;

        const std::vector<std::uint8_t>& getRawBuffer() const { return m_buffer; }
    };

}

#endif 