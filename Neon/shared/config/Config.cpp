#include "Config.h"

#include <fstream>
#include <stdexcept>
#include <nlohmann/json.hpp>

namespace shared::config
{
    using nlohmann::json;

    static void from_json(const json& j, Engine& e)
    {
        j.at("id").get_to(e.id);
        j.at("displacement_l").get_to(e.displacement_l);
        j.at("idle_rpm").get_to(e.idle_rpm);
        j.at("max_rpm").get_to(e.max_rpm);
        j.at("max_torque_nm").get_to(e.max_torque_nm);
    }

    static void from_json(const json& j, Transmission& t)
    {
        j.at("id").get_to(t.m_id);
        j.at("gears").get_to(t.m_gears);
        j.at("final_drive").get_to(t.m_finalDrive);
        j.at("gear_ratios").get_to(t.m_gearRatios);
        j.at("reverse_ratio").get_to(t.m_reverseRatio);

        if (static_cast<int>(t.m_gearRatios.size()) != t.m_gears)
            throw std::runtime_error("Config: transmission.gear_ratios length must equal transmission.gears");
    }

    static void from_json(const json& j, VehicleConfig& v)
    {
        from_json(j.at("engine"), v.m_engine);
        from_json(j.at("transmission"), v.m_transmission);
    }

    void Config::LoadFromFile(const std::string& path)
    {
        std::ifstream file(path);
        if (!file.is_open())
            throw std::runtime_error("Config: failed to open file: " + path);

        json j;
        file >> j;

        m_config = {};
        from_json(j, m_config);
    }

    const Engine& Config::getEngineConfig() const
    {
        return m_config.m_engine;
    }

    const Transmission& Config::getTransmissionConfig() const
    {
        return m_config.m_transmission;
    }
}
