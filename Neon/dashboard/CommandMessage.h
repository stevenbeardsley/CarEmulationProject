#ifndef DASHBOARD_COMMANDMESSAGE_H
#define DASHBOARD_COMMANDMESSAGE_H

#include <string>

namespace dashboard
{

enum class Command
{
    GearUp,
    GearDown,
    Throttle,
    Fuel,
    Refuel,
    Unknown
};


static Command toCommand(const std::string& command)
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
    else if (command == "refuel")
    {
        commandType = Command::Refuel;
    }
	return commandType;
}

}
#endif