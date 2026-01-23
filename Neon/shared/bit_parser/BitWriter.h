#pragma once 

#include "BitOrder.h"
#include "Error.h"
#include "ByteOrder.h"
#include "ByteBuffer.h"
#include "Span.h"
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

    class BitWriter {
    public:
        explicit BitWriter(ByteBuffer& out,
            BitOrder bitOrder = BitOrder::MsbFirst,
            ByteOrder byteOrder = ByteOrder::BigEndian)
            : out_(&out), bitOrder_(bitOrder), byteOrder_(byteOrder) {
        }

        void reset() {
            bitPos_ = 0;
            err_ = Error::None;
            out_->clear();
        }

        bool ok() const { return err_ == Error::None; }
        Error error() const { return err_; }

        size_t bitPosition() const { return bitPos_; }
        size_t bytePosition() const { return bitPos_ / 8; }

        // Ensure buffer is big enough for bitPos_ (and upcoming writes)
        void ensureBits(size_t bitsNeeded) {
            const size_t totalBits = bitPos_ + bitsNeeded;
            const size_t requiredBytes = (totalBits + 7) / 8;
            if (out_->size() < requiredBytes) out_->resize(requiredBytes, 0);
        }

        // Write lowest 'bitCount' bits of value into the stream.
        // Example: writeBits(0b101, 3) writes bits: 1,0,1 (in that order)
        void writeBits(uint64_t value, size_t bitCount) {
            if (!ok()) return;
            if (bitCount > 64) { err_ = Error::OutOfRange; return; }

            ensureBits(bitCount);

            for (size_t i = 0; i < bitCount; ++i) {
                // Write from MSB of the field to LSB? We choose "field MSB first":
                // i=0 writes the (bitCount-1)th bit, i=bitCount-1 writes bit 0.
                const size_t srcShift = (bitCount - 1) - i;
                const uint8_t bit = static_cast<uint8_t>((value >> srcShift) & 0x1u);

                const size_t byteIndex = bitPos_ / 8;
                const uint8_t bitIndexInByte = static_cast<uint8_t>(bitPos_ % 8);
                const uint8_t mask = bitMaskInByte(bitIndexInByte, bitOrder_);

                uint8_t& dstByte = out_->vec()[byteIndex];
                if (bit) dstByte |= mask;
                else     dstByte &= static_cast<uint8_t>(~mask);

                ++bitPos_;
            }
        }

        void writeBool(bool b) { writeBits(b ? 1u : 0u, 1); }

        void alignToByte(uint8_t padBit = 0) {
            if (!ok()) return;
            const size_t mod = bitPos_ % 8;
            if (mod == 0) return;
            const size_t pad = 8 - mod;
            for (size_t i = 0; i < pad; ++i) writeBits(padBit ? 1u : 0u, 1);
        }

        // Fixed-width helpers (unsigned only, minimal)
        void writeU8(uint8_t v) { writeBits(v, 8); }

        void writeU16(uint16_t v) {
            if (byteOrder_ == ByteOrder::BigEndian) {
                writeBits((v >> 8) & 0xFFu, 8);
                writeBits((v >> 0) & 0xFFu, 8);
            }
            else {
                writeBits((v >> 0) & 0xFFu, 8);
                writeBits((v >> 8) & 0xFFu, 8);
            }
        }

        void writeU32(uint32_t v) {
            if (byteOrder_ == ByteOrder::BigEndian) {
                writeU16(static_cast<uint16_t>((v >> 16) & 0xFFFFu));
                writeU16(static_cast<uint16_t>((v >> 0) & 0xFFFFu));
            }
            else {
                writeU16(static_cast<uint16_t>((v >> 0) & 0xFFFFu));
                writeU16(static_cast<uint16_t>((v >> 16) & 0xFFFFu));
            }
        }

        void writeU64(uint64_t v) {
            if (byteOrder_ == ByteOrder::BigEndian) {
                writeU32(static_cast<uint32_t>((v >> 32) & 0xFFFFFFFFull));
                writeU32(static_cast<uint32_t>((v >> 0) & 0xFFFFFFFFull));
            }
            else {
                writeU32(static_cast<uint32_t>((v >> 0) & 0xFFFFFFFFull));
                writeU32(static_cast<uint32_t>((v >> 32) & 0xFFFFFFFFull));
            }
        }

        // VarUInt (7-bit groups, continuation bit)
        // Always byte-aligned for simplicity + sanity.
        void writeVarUInt(uint64_t v) {
            if (!ok()) return;
            alignToByte();

            // Standard LEB128-like varint (little-endian groups).
            // This is independent of byteOrder_.
            do {
                uint8_t byte = static_cast<uint8_t>(v & 0x7Fu);
                v >>= 7;
                if (v != 0) byte |= 0x80u;
                writeU8(byte);
            } while (v != 0);
        }

        void writeBytes(Span<const uint8_t> bytes) {
            if (!ok()) return;
            if ((bitPos_ % 8) != 0) { err_ = Error::Misaligned; return; }
            ensureBits(bytes.size() * 8);
            for (uint8_t b : bytes) writeU8(b);
        }

        void writeString(std::string_view s) {
            if (!ok()) return;
            writeVarUInt(static_cast<uint64_t>(s.size()));
            // writeVarUInt aligns to byte; writeBytes requires alignment too.
            writeBytes(Span<const uint8_t>(
                reinterpret_cast<const uint8_t*>(s.data()), s.size()
            ));
        }

    private:
        ByteBuffer* out_ = nullptr;
        size_t bitPos_ = 0;
        Error err_ = Error::None;
        BitOrder bitOrder_ = BitOrder::MsbFirst;
        ByteOrder byteOrder_ = ByteOrder::BigEndian;
    };
}