#pragma once
#include <string>

namespace dashboard
{

enum class Command
{
    GearUp,
    GearDown,
    Unknown
};


static Command toCommand(std::string command)
{
    auto commandType = Command::Unknown;
    if (command == "gear_up")
    {
        commandType = Command::GearUp;
    }

    return commandType;
}

}