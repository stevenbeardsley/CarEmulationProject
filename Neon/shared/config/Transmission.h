#ifndef SHARED_CONFIG_TRANSMISSION_H
#define SHARED_CONFIG_TRANSMISSION_H

#include <string>
#include <vector>

namespace shared::config
{

struct Transmission
{
    std::string m_id;
    int m_gears{};
    double m_finalDrive{};
    std::vector<double> m_gearRatios;
    double m_reverseRatio{};
};
}

#endif