#pragma once 

#include "ByteBuffer.h"
#include "BitOrder.h"
#include "ByteOrder.h"
#include "Span.h"
#include "Error.h"
#include <cstdint>
#include <vector>
#include <string>
#include <string_view>
#include <span>
#include <limits>

namespace shared::bit_parser
{
    inline uint8_t bitMaskInByte(uint8_t bitIndex0to7, BitOrder bo) {
        // For MsbFirst: bitIndex 0 -> mask 0x80, bitIndex 7 -> mask 0x01
        // For LsbFirst: bitIndex 0 -> mask 0x01, bitIndex 7 -> mask 0x80
        return (bo == BitOrder::MsbFirst)
            ? static_cast<uint8_t>(0x80u >> bitIndex0to7)
            : static_cast<uint8_t>(0x01u << bitIndex0to7);
    }
class BitReader {
public:
    explicit BitReader(Span<const uint8_t> in,
        BitOrder bitOrder = BitOrder::MsbFirst,
        ByteOrder byteOrder = ByteOrder::BigEndian)
        : in_(in), bitOrder_(bitOrder), byteOrder_(byteOrder) {
    }

    bool ok() const { return err_ == Error::None; }
    Error error() const { return err_; }

    size_t bitPosition() const { return bitPos_; }
    size_t remainingBits() const { return (in_.size() * 8) - bitPos_; }

    void alignToByte() {
        if (!ok()) return;
        const size_t mod = bitPos_ % 8;
        if (mod == 0) return;
        const size_t skip = 8 - mod;
        if (remainingBits() < skip) { err_ = Error::OutOfRange; return; }
        bitPos_ += skip;
    }

    uint64_t readBits(size_t bitCount) {
        if (!ok()) return 0;
        if (bitCount > 64) { err_ = Error::OutOfRange; return 0; }
        if (remainingBits() < bitCount) { err_ = Error::OutOfRange; return 0; }

        uint64_t out = 0;
        for (size_t i = 0; i < bitCount; ++i) {
            const size_t byteIndex = bitPos_ / 8;
            const uint8_t bitIndexInByte = static_cast<uint8_t>(bitPos_ % 8);
            const uint8_t mask = bitMaskInByte(bitIndexInByte, bitOrder_);
            const uint8_t bit = (in_[byteIndex] & mask) ? 1u : 0u;

            // Field MSB-first reconstruction:
            out = (out << 1) | bit;
            ++bitPos_;
        }
        return out;
    }

    bool readBool() { return readBits(1) != 0; }

    uint8_t readU8() { return static_cast<uint8_t>(readBits(8)); }

    uint16_t readU16() {
        uint16_t a = static_cast<uint16_t>(readBits(8));
        uint16_t b = static_cast<uint16_t>(readBits(8));
        return (byteOrder_ == ByteOrder::BigEndian)
            ? static_cast<uint16_t>((a << 8) | b)
            : static_cast<uint16_t>((b << 8) | a);
    }

    uint32_t readU32() {
        if (byteOrder_ == ByteOrder::BigEndian) {
            uint32_t hi = readU16();
            uint32_t lo = readU16();
            return (hi << 16) | lo;
        }
        else {
            uint32_t lo = readU16();
            uint32_t hi = readU16();
            return (hi << 16) | lo;
        }
    }

    uint64_t readU64() {
        if (byteOrder_ == ByteOrder::BigEndian) {
            uint64_t hi = readU32();
            uint64_t lo = readU32();
            return (hi << 32) | lo;
        }
        else {
            uint64_t lo = readU32();
            uint64_t hi = readU32();
            return (hi << 32) | lo;
        }
    }

    uint64_t readVarUInt() {
        if (!ok()) return 0;
        alignToByte();
        if (!ok()) return 0;

        uint64_t value = 0;
        uint32_t shift = 0;

        // Up to 10 bytes for 64-bit varint
        for (int i = 0; i < 10; ++i) {
            if (remainingBits() < 8) { err_ = Error::OutOfRange; return 0; }
            const uint8_t byte = readU8();

            const uint64_t chunk = static_cast<uint64_t>(byte & 0x7Fu);
            if (shift >= 64 && chunk != 0) { err_ = Error::BadVarUInt; return 0; }

            value |= (chunk << shift);
            if ((byte & 0x80u) == 0) return value;

            shift += 7;
        }

        err_ = Error::BadVarUInt;
        return 0;
    }

    std::vector<uint8_t> readBytes(size_t n) {
        if (!ok()) return {};
        if ((bitPos_ % 8) != 0) { err_ = Error::Misaligned; return {}; }
        if (remainingBits() < n * 8) { err_ = Error::OutOfRange; return {}; }

        std::vector<uint8_t> out;
        out.reserve(n);
        for (size_t i = 0; i < n; ++i) out.push_back(readU8());
        return out;
    }

    std::string readString() {
        if (!ok()) return {};
        const uint64_t len64 = readVarUInt();
        if (!ok()) return {};
        if (len64 > static_cast<uint64_t>(std::numeric_limits<size_t>::max())) {
            err_ = Error::OutOfRange;
            return {};
        }
        const size_t len = static_cast<size_t>(len64);
        auto bytes = readBytes(len);
        if (!ok()) return {};
        return std::string(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    }

private:
    Span<const uint8_t> in_;
    size_t bitPos_ = 0;
    Error err_ = Error::None;
    BitOrder bitOrder_ = BitOrder::MsbFirst;
    ByteOrder byteOrder_ = ByteOrder::BigEndian;
};

}