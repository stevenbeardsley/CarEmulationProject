#ifndef SHARED_CAN_MESSAGE_ICANMESSAGE_H
#define SHARED_CAN_MESSAGE_ICANMESSAGE_H

#include <vector>
#include <cstdint>

namespace shared::can::message
{
class ICanMessage
{
protected:
    std::vector<std::uint8_t> m_buffer;

public:
    virtual ~ICanMessage() = default;

    [[nodiscard]] 
	const std::vector<std::uint8_t>& getRawData() const { return m_buffer; }

    operator const std::vector<std::uint8_t>& () const
    {
        return m_buffer;
    }
};
}

#endif