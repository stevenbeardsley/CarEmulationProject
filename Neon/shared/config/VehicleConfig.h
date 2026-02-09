#ifndef SHARED_CONFIG_VEHICLECONFIG_H
#define SHARED_CONFIG_VEHICLECONFIG_H

#include "Engine.h"
#include "Transmission.h"

namespace shared::config
{
struct VehicleConfig
{
	Engine m_engine;
	Transmission m_transmission;
};
}

#endif