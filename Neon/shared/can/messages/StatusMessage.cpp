#include "StatusMessage.h"

#include "../bit_parser/ByteBuffer.h"
#include "../bit_parser/BitWriter.h"
#include "../bit_parser/BitReader.h"

#include <vector>
#include <cstdint>

namespace shared::can::message
{
    StatusMessage::StatusMessage(std::vector<std::uint8_t> existingBuffer)
    {
        m_buffer = std::move(existingBuffer);
    }

    StatusMessage::StatusMessage(MessageCategory category, headers::Status type, std::uint32_t value)
    {
        // encode: [category: u8][type: u8][value: u32]
        m_buffer.clear();
        m_buffer.reserve(1u + 1u + 4u); // Total 6 bytes

        shared::bit_parser::ByteBuffer buf(m_buffer);
        shared::bit_parser::BitWriter w(buf);

        w.writeU8(static_cast<std::uint8_t>(category));
        w.writeU8(static_cast<std::uint8_t>(type));
        w.writeU32(value);
    }

    MessageCategory StatusMessage::getCategory() const
    {
        shared::bit_parser::BitReader r(
            shared::bit_parser::Span<const std::uint8_t>(m_buffer.data(), m_buffer.size())
        );

        return static_cast<MessageCategory>(r.readU8());
    }

    headers::Status StatusMessage::getType() const
    {
        shared::bit_parser::BitReader r(
            shared::bit_parser::Span<const std::uint8_t>(m_buffer.data(), m_buffer.size())
        );

        (void)r.readU8(); // Skip Category
        const std::uint8_t rawType = r.readU8();
        return static_cast<headers::Status>(rawType);
    }

    std::uint32_t StatusMessage::getValue() const
    {
        shared::bit_parser::BitReader r(
            shared::bit_parser::Span<const std::uint8_t>(m_buffer.data(), m_buffer.size())
        );

        (void)r.readU8(); // Skip Category
        (void)r.readU8(); // Skip Type
        return r.readU32();
    }

}
