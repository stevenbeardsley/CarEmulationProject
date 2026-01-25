#include "Message.h"

// adjust include paths to your project
#include "shared/bit_parser/ByteBuffer.h"
#include "shared/bit_parser/BitWriter.h"
#include "shared/bit_parser/BitReader.h"

#include <vector>
#include <cstdint>

namespace shared::can
{
Message::Message(MessageType type, std::uint32_t value)
{
    // encode: [type: u8][value: u32]
    m_buffer.clear();
    m_buffer.reserve(1u + 4u);

    // ByteBuffer now wraps external storage (your updated design)
    shared::bit_parser::ByteBuffer buf(m_buffer);
    shared::bit_parser::BitWriter w(buf);

    w.writeU8(static_cast<std::uint8_t>(type));
    w.writeU32(value);

    // assume valid, no error handling
}

MessageType Message::getMessageType() const
{
    shared::bit_parser::BitReader r(
        shared::bit_parser::Span<const std::uint8_t>(m_buffer.data(), m_buffer.size())
    );

    const std::uint8_t rawType = r.readU8();
    return static_cast<MessageType>(rawType);
}

std::uint32_t Message::getValue() const
{
    shared::bit_parser::BitReader r(
        shared::bit_parser::Span<const std::uint8_t>(m_buffer.data(), m_buffer.size())
    );

    (void)r.readU8();               // skip type
    const std::uint32_t value = r.readU32();
    return value;
}

} // namespace shared::can
