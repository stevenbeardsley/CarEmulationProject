#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <cstdint>

#include "shared/can/messages/ErrorMessage.h" 
#include "shared/can/headers/Error.h"

using namespace shared::can;

class ErrorMessageTest : public ::testing::Test {
protected:
};

TEST_F(ErrorMessageTest, ConstructsFromParametersAndReadsCorrectly)
{
    auto expectedCategory = static_cast<MessageCategory>(3);
    auto expectedType = static_cast<headers::Error>(12);
    std::string expectedMsg = "Sensor connection timeout";

    message::ErrorMessage msg(expectedCategory, expectedType, expectedMsg);

    EXPECT_EQ(msg.getCategory(), expectedCategory);
    EXPECT_EQ(msg.getErrorType(), expectedType);
    EXPECT_EQ(msg.getErrorMessage(), expectedMsg);
}

TEST_F(ErrorMessageTest, HandlesEmptyErrorMessageString)
{
    auto expectedCategory = static_cast<MessageCategory>(1);
    auto expectedType = static_cast<headers::Error>(5);
    std::string expectedMsg = "";

    message::ErrorMessage msg(expectedCategory, expectedType, expectedMsg);

    EXPECT_EQ(msg.getCategory(), expectedCategory);
    EXPECT_EQ(msg.getErrorType(), expectedType);
    EXPECT_EQ(msg.getErrorMessage(), expectedMsg);
}

TEST_F(ErrorMessageTest, ConstructsFromExistingBufferAndReadsCorrectly)
{
    std::vector<std::uint8_t> rawBuffer = {
        0x04,
        0x08,
        0x05,
        'E', 'r', 'r', 'o', 'r'
    };

    message::ErrorMessage msg(std::move(rawBuffer));

    EXPECT_EQ(static_cast<std::uint8_t>(msg.getCategory()), 0x04);
    EXPECT_EQ(static_cast<std::uint8_t>(msg.getErrorType()), 0x08);
    EXPECT_EQ(msg.getErrorMessage(), "Error");
}