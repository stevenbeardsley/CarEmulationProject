#include <gtest/gtest.h>
#include <vector>
#include <cstdint>

#include "shared/can/messages/SpeedControl.h"
#include "shared/can/headers/Control.h"

using namespace shared::can;

class ThrottleTest : public ::testing::Test {
protected:
};

TEST_F(ThrottleTest, ConstructsFromParametersAndReadsCorrectly)
{
    auto expectedCategory = static_cast<MessageCategory>(5);
    auto expectedType = static_cast<headers::Control>(2);
    std::uint32_t expectedValue = 987654321;

    message::SpeedControl msg(expectedCategory, expectedType, expectedValue);

    EXPECT_EQ(msg.getCategory(), expectedCategory);
    EXPECT_EQ(msg.getType(), expectedType);
    EXPECT_EQ(msg.getValue(), expectedValue);
}

TEST_F(ThrottleTest, ConstructsFromExistingBufferAndReadsCorrectly) /// Byte endianess  
{
    std::vector<std::uint8_t> rawBuffer = {
        0x03,
        0x01,
        0xAA, 0xBB, 0xCC, 0xDD
    };

    message::SpeedControl msg(std::move(rawBuffer));

    EXPECT_EQ(static_cast<std::uint8_t>(msg.getCategory()), 0x03);
    EXPECT_EQ(static_cast<std::uint8_t>(msg.getType()), 0x01);
    EXPECT_EQ(msg.getValue(), 0xAABBCCDD);
}