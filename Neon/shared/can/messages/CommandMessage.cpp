#include "CommandMessage.h"

#include "../bit_parser/ByteBuffer.h"
#include "../bit_parser/BitWriter.h"
#include "../bit_parser/BitReader.h"

#include <vector>
#include <cstdint>

namespace shared::can::message
{
    CommandMessage::CommandMessage(std::vector<std::uint8_t> existingBuffer)
    {
        m_buffer = std::move(existingBuffer);
    }

    CommandMessage::CommandMessage(MessageCategory category, headers::Control type)
    {
        // encode: [category: u8][type: u8]
        m_buffer.clear();
        m_buffer.reserve(1u + 1u); // Total 2 bytes

        shared::bit_parser::ByteBuffer buf(m_buffer);
        shared::bit_parser::BitWriter w(buf);

        w.writeU8(static_cast<std::uint8_t>(category));
        w.writeU8(static_cast<std::uint8_t>(type));
    }

    MessageCategory CommandMessage::getCategory() const
    {
        shared::bit_parser::BitReader r(
            shared::bit_parser::Span<const std::uint8_t>(m_buffer.data(), m_buffer.size())
        );

        return static_cast<MessageCategory>(r.readU8());
    }

    headers::Control CommandMessage::getType() const
    {
        shared::bit_parser::BitReader r(
            shared::bit_parser::Span<const std::uint8_t>(m_buffer.data(), m_buffer.size())
        );

        (void)r.readU8(); // Skip Category
        const auto rawType = r.readU8();
        return static_cast<headers::Control>(rawType);
    }

}
