#ifndef SHARED_CONFIG_H
#define SHARED_CONFIG_H

#include "VehicleConfig.h"

namespace shared::config
{

class Config
{
public:
    // Load config.json from disk into this Config instance
    // Throws std::runtime_error / nlohmann::json exceptions if invalid.
    void LoadFromFile(const std::string& path);

    // Accessors (no copies)
    const Engine& getEngineConfig() const;
    const Transmission& getTransmissionConfig() const;


private:
    VehicleConfig m_config{};
};

}

#endif
