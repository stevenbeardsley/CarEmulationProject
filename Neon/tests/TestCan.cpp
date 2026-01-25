#include <gtest/gtest.h>

#include "shared/can/Message.h"
#include "shared/can/MessageType.h"

#include <cstdint>

namespace shared::can
{
    TEST(CanMessage, Gear100_RoundTripsViaAccessors)
    {
        Message msg(MessageType::Gear, 100u);

        EXPECT_EQ(msg.getMessageType(), MessageType::Gear);
        EXPECT_EQ(msg.getValue(), 100u);
    }

    TEST(CanMessage, AccessorsRoundTripVariousValues)
    {
        struct Case
        {
            MessageType type;
            std::uint32_t value;
        };

        const Case cases[] = {
            { MessageType::Gear, 0u },
            { MessageType::Gear, 1u },
            { MessageType::Gear, 100u },
            { MessageType::Gear, 0xFFFFFFFFu },
        };

        for (const auto& c : cases)
        {
            Message msg(c.type, c.value);
            EXPECT_EQ(msg.getMessageType(), c.type);
            EXPECT_EQ(msg.getValue(), c.value);
        }
    }

    TEST(CanMessage, DifferentMessageTypes_RoundTripViaAccessors)
    {
        // This test assumes these enum values exist in your MessageType.
        // If you only have Gear right now, remove the extra cases.
        const MessageType typesToTry[] = {
            MessageType::Gear,
            MessageType::Speed,
            MessageType::RPM
        };

        for (auto t : typesToTry)
        {
            Message msg(t, 123u);
            EXPECT_EQ(msg.getMessageType(), t);
            EXPECT_EQ(msg.getValue(), 123u);
        }
    }
} // namespace shared::can
