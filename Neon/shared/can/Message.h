#ifndef SHARED_CAN_MESSAGE_H
#define SHARED_CAN_MESSAGE_H

#include "MessageType.h"
#include <cstdint>
#include <vector>

namespace shared::can
{

class Message
{
public:
	Message() = delete; // Can't make an empty message 
	
	Message(MessageType type,
		std::uint32_t value);  

	MessageType getMessageType() const;

	std::uint32_t getValue() const;

private:
	std::vector<std::uint8_t> m_buffer;
};

}

#endif 