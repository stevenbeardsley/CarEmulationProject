#include "Transmission.h"
#include "LogFile.h"

namespace tcm::transmission
{

std::uint32_t Transmission::getGear() const
{
	std::lock_guard<std::mutex> lk(m_lock); // Lock so the value cannot change
	return m_gear;
}

void Transmission::gearUp()
{
	LogFile::Info("Gear has been shifted up."); // TODO: Logging shouldn't really be here as such 
	m_gear++;
}

void Transmission::gearDown()
{
	LogFile::Info("Gear has been shifted down");
	m_gear--;
}

}