#ifndef TCM_TRANSMISSION_TRANSMISSION_H
#define TCM_TRANSMISSION_TRANSMISSION_H

#include <cstdint>
#include <mutex>

namespace tcm::transmission
{

class Transmission
{
public:
	Transmission() = default;
	
	void gearUp();

	void gearDown();
	
	[[nodiscard]]
	std::uint32_t getGear() const;
	// TODO: Implement more tricky physics in this class as the actual transmission "box"
private:
	mutable std::mutex m_lock;
	std::uint32_t m_gear{ 0 };
};
}

#endif 