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
    else if (command == "gear_down")
    {
        commandType = Command::GearDown;
    }
    else if (command == "throttle")
    {
        commandType = Command::Throttle;
    }

    return commandType;
}

}