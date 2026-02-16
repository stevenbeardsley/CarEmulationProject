#include "Transmission.h"
#include "LogFile.h"

namespace tcm::transmission
{

Transmission::Transmission(const shared::config::Engine& engineConfig, const shared::config::Transmission& transmissionConfig):
	m_engineConfig(engineConfig),
	m_transmissionConfig(transmissionConfig)
{
	LogFile::Info("Transmission: Transmission system created with id: " + m_transmissionConfig.m_id);
}

std::uint32_t Transmission::getGear() const
{
	std::lock_guard<std::mutex> lk(m_lock); // Lock so the value cannot change
	return m_gear;
}

void Transmission::gearUp()
{
	m_gear++;
	LogFile::Info("Transmission: Gear has been shifted up."); 
}

void Transmission::gearDown()
{
	if (m_gear == 0)
	{
		LogFile::Error("Transmission: Gear is at the bottom gear, cannot be shifted lower.");
	}
	else
	{
		m_gear--;
		LogFile::Info("Transmission: Gear has been shifted down");
	}
}

}