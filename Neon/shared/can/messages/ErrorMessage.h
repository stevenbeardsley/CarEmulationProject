#ifndef SHARED_CAN_MESSAGE_ERRORMESSAGE_H
#define SHARED_CAN_MESSAGE_ERRORMESSAGE_H

#include <vector>
#include <cstdint>
#include <string>

#include "ICanMessage.h"
#include "MessageCategory.h"
#include "headers/Error.h"

namespace shared::can::message
{

    class ErrorMessage : public ICanMessage
    {
    public:
        explicit ErrorMessage(std::vector<std::uint8_t> existingBuffer);

        ErrorMessage(MessageCategory category, headers::Error type, const std::string& errorMessage);

        MessageCategory getCategory() const;
        headers::Error getErrorType() const;
        std::string getErrorMessage() const;

    };
}

#endif