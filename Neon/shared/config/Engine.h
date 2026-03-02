#ifndef SHARED_CONFIG_ENGINE_H
#define SHARED_CONFIG_ENGINE_H

#include <string>

namespace shared::config
{

struct Engine
{
    std::string id;
    double displacement_l{};
    int idle_rpm{};
    int max_rpm{};
    double max_torque_nm{};
};

}

#endif 