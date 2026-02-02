#pragma once
#include <string>

namespace dashboard
{

enum class Command
{
    GearUp,
    GearDown,
    Throttle,
    Unknown
};


static Command toCommand(std::string command)
{
    auto commandType = Command::Unknown;
    if (command == "gear_up")
    {
        commandType = Command::GearUp;
    }
    if (command == "gear_down")
    {
        commandType = Command::GearDown;
    }
    if (command == "throttle")
    {
        commandType = Command::Throttle;
    }

    return commandType;
}

}