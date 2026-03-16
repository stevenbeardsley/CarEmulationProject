#include "ErrorMessage.h"

#include "../bit_parser/ByteBuffer.h"
#include "../bit_parser/BitWriter.h"
#include "../bit_parser/BitReader.h"

namespace shared::can::message
{
    ErrorMessage::ErrorMessage(std::vector<std::uint8_t> existingBuffer)
    {
        m_buffer = std::move(existingBuffer);
    }

    ErrorMessage::ErrorMessage(MessageCategory category, headers::Error type, const std::string& errorMessage)
    {
        // encode: [category: u8][type: u8][string_length: varUInt][string_chars: u8...]
        m_buffer.clear();

        m_buffer.reserve(1u + 1u + 9u + errorMessage.length());

        bit_parser::ByteBuffer buf(m_buffer);
        bit_parser::BitWriter w(buf);

        w.writeU8(static_cast<std::uint8_t>(category));

        w.writeU8(static_cast<std::uint8_t>(type));

        w.writeString(errorMessage);
    }

    MessageCategory ErrorMessage::getCategory() const
    {
        bit_parser::BitReader r(
            bit_parser::Span<const std::uint8_t>(m_buffer.data(), m_buffer.size())
        );

        return static_cast<MessageCategory>(r.readU8());
    }

    headers::Error ErrorMessage::getErrorType() const
    {
        bit_parser::BitReader r(
            bit_parser::Span<const std::uint8_t>(m_buffer.data(), m_buffer.size())
        );

        (void)r.readU8(); // Skip MessageCategory

        return static_cast<headers::Error>(r.readU8());
    }

    std::string ErrorMessage::getErrorMessage() const
    {
        bit_parser::BitReader r(
            bit_parser::Span<const std::uint8_t>(m_buffer.data(), m_buffer.size())
        );

        (void)r.readU8(); // Skip MessageCategory
        (void)r.readU8(); // Skip ErrorType

        return r.readString();
    }
}