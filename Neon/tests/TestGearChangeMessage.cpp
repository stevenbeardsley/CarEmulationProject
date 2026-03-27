#include <gtest/gtest.h>
#include <vector>
#include <cstdint>

#include "shared/can/messages/CommandMessage.h"
#include "shared/can/headers/Control.h"

using namespace shared::can;

class GearChangeTest : public ::testing::Test {
protected:
};

TEST_F(GearChangeTest, ConstructsFromParametersAndReadsCorrectly)
{
    auto expectedCategory = static_cast<MessageCategory>(2);
    auto expectedType = static_cast<headers::Control>(5);

    message::CommandMessage msg(expectedCategory, expectedType);

    EXPECT_EQ(msg.getCategory(), expectedCategory);
    EXPECT_EQ(msg.getType(), expectedType);
}

TEST_F(GearChangeTest, ConstructsFromExistingBufferAndReadsCorrectly)
{
    std::vector<std::uint8_t> rawBuffer = {
        0x07,
        0x03
    };

    message::CommandMessage msg(std::move(rawBuffer));

    EXPECT_EQ(static_cast<std::uint8_t>(msg.getCategory()), 0x07);
    EXPECT_EQ(static_cast<std::uint8_t>(msg.getType()), 0x03);
}