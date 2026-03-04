#include <gtest/gtest.h>
#include <vector>
#include <cstdint>

#include "shared/can/messages/StatusMessage.h"
#include "shared/can/headers/Status.h"

using namespace shared::can;

class StatusMessageTest : public ::testing::Test {
protected:
};

TEST_F(StatusMessageTest, ConstructsFromParametersAndReadsCorrectly)
{
    auto expectedCategory = static_cast<MessageCategory>(4);
    auto expectedType = static_cast<headers::Status>(9);
    std::uint32_t expectedValue = 8675309;

    message::StatusMessage msg(expectedCategory, expectedType, expectedValue);

    EXPECT_EQ(msg.getCategory(), expectedCategory);
    EXPECT_EQ(msg.getType(), expectedType);
    EXPECT_EQ(msg.getValue(), expectedValue);
}

TEST_F(StatusMessageTest, ConstructsFromExistingBufferAndReadsCorrectly)
{
    std::vector<std::uint8_t> rawBuffer = {
        0x02,
        0x0A,
        0x12, 0x34, 0x56, 0x78
    };

    message::StatusMessage msg(std::move(rawBuffer));

    EXPECT_EQ(static_cast<std::uint8_t>(msg.getCategory()), 0x02);
    EXPECT_EQ(static_cast<std::uint8_t>(msg.getType()), 0x0A);
    EXPECT_EQ(msg.getValue(), 0x12345678);
}