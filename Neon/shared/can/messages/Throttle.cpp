#include "Throttle.h"

#include "../bit_parser/ByteBuffer.h"
#include "../bit_parser/BitWriter.h"
#include "../bit_parser/BitReader.h"

namespace shared::can::message
{
    Throttle::Throttle(MessageCategory category, headers::Control type, std::uint32_t value)
    {
        // encode: [category: u8][type: u8][value: u32]
        m_buffer.clear();
        m_buffer.reserve(1u + 1u + 4u); // Total 6 bytes

        bit_parser::ByteBuffer buf(m_buffer);
        bit_parser::BitWriter w(buf);

        w.writeU8(static_cast<std::uint8_t>(category));
        w.writeU8(static_cast<std::uint8_t>(type));
        w.writeU32(value);
    }

    Throttle::Throttle(std::vector<std::uint8_t> existingBuffer)
    {
        m_buffer = std::move(existingBuffer); 
    }

    MessageCategory Throttle::getCategory() const
    {
        bit_parser::BitReader r(
            shared::bit_parser::Span<const std::uint8_t>(m_buffer.data(), m_buffer.size())
        );

        return static_cast<MessageCategory>(r.readU8());
    }

    headers::Control Throttle::getType() const
    {
        bit_parser::BitReader r(
            shared::bit_parser::Span<const std::uint8_t>(m_buffer.data(), m_buffer.size())
        );

        (void)r.readU8(); // Skip MessageCategory

        return static_cast<headers::Control>(r.readU8());
    }

    std::uint32_t Throttle::getValue() const
    {
        bit_parser::BitReader r(
            shared::bit_parser::Span<const std::uint8_t>(m_buffer.data(), m_buffer.size())
        );

        (void)r.readU8(); // Skip Category
        (void)r.readU8(); // Skip Type
        return r.readU32();
    }
}