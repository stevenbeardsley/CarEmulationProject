#ifndef CONFIG_TRANSMISSION_H
#define CONFIG_TRANSMISSION_H
#include <string>
#include <vector>

namespace shared::config
{

struct Transmission
{
    std::string id;
    int gears{};
    double final_drive{};
    std::vector<double> gear_ratios;
    double reverse_ratio{};
};
}

#endif