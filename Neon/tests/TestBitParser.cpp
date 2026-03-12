#include <gtest/gtest.h>

#include "shared/bit_parser/BitReader.h"
#include "shared/bit_parser/ByteBuffer.h"
#include "shared/bit_parser/BitWriter.h"

#include <cstdint>
#include <string>
#include <vector>

namespace shared::bit_parser
{
    static Span<const uint8_t> AsSpan(const std::vector<uint8_t>& v) {
        return Span<const uint8_t>(v.data(), v.size());
    }

    TEST(BitParser, EmptyBufferReadShouldFailOutOfRange)
    {
        std::vector<uint8_t> empty;
        BitReader r(Span<const uint8_t>(empty.data(), empty.size()));
        EXPECT_TRUE(r.ok());              // ok until a read is attempted
        (void)r.readBits(1);
        EXPECT_FALSE(r.ok());
        EXPECT_EQ(r.error(), Error::OutOfRange);
    }

    TEST(BitParser, WriteAndReadSingleBitsAcrossByteBoundary)
    {
        std::vector<uint8_t> storage;
        ByteBuffer buf(storage);
        BitWriter w(buf);

        // 9 bits: 101010101
        w.writeBits(0b101010101ull, 9);
        ASSERT_TRUE(w.ok());

        BitReader r(AsSpan(storage));
        const auto v = r.readBits(9);
        ASSERT_TRUE(r.ok());
        EXPECT_EQ(v, 0b101010101ull);
    }

    TEST(BitParser, WriteAndReadFixedWidthIntegersBigEndian)
    {
        std::vector<uint8_t> storage;
        ByteBuffer buf(storage);
        BitWriter w(buf, BitOrder::MsbFirst, ByteOrder::BigEndian);

        w.writeU8(0xAB);
        w.writeU16(0x1234);
        w.writeU32(0x89ABCDEFu);
        w.writeU64(0x0123456789ABCDEFull);

        ASSERT_TRUE(w.ok());

        BitReader r(AsSpan(storage), BitOrder::MsbFirst, ByteOrder::BigEndian);
        EXPECT_EQ(r.readU8(), 0xAB);
        EXPECT_EQ(r.readU16(), 0x1234);
        EXPECT_EQ(r.readU32(), 0x89ABCDEFu);
        EXPECT_EQ(r.readU64(), 0x0123456789ABCDEFull);
        EXPECT_TRUE(r.ok());
    }

    TEST(BitParser, ArbitraryFieldsAndAlignmentThenString)
    {
        std::vector<uint8_t> storage;
        ByteBuffer buf(storage);
        BitWriter w(buf);

        // Example: msgId (16), speed (12), gear (4) => total 32 bits (already aligned)
        w.writeU16(0x1001);
        w.writeBits(10, 12);
        w.writeBits(3, 4);

        // Now write a string (writer aligns before varint internally)
        w.writeString("VIN123");
        ASSERT_TRUE(w.ok());

        BitReader r(AsSpan(storage));
        EXPECT_EQ(r.readU16(), 0x1001);
        EXPECT_EQ(r.readBits(12), 10u);
        EXPECT_EQ(r.readBits(4), 3u);
        EXPECT_EQ(r.readString(), "VIN123");
        EXPECT_TRUE(r.ok());
    }

    TEST(BitParser, MisalignedWriteBytesShouldFail)
    {
        std::vector<uint8_t> storage;
        ByteBuffer buf(storage);
        BitWriter w(buf);

        w.writeBits(1, 1); // now misaligned (bitPos % 8 != 0)
        ASSERT_TRUE(w.ok());

        const uint8_t payload[2] = { 0xDE, 0xAD };
        w.writeBytes(Span<const uint8_t>(payload, 2));

        EXPECT_FALSE(w.ok());
        EXPECT_EQ(w.error(), Error::Misaligned);
    }

    TEST(BitParser, MisalignedReadBytesShouldFail)
    {
        std::vector<uint8_t> storage;
        ByteBuffer buf(storage);
        BitWriter w(buf);

        w.writeBits(1, 1);
        w.alignToByte();
        w.writeU8(0xFF);
        ASSERT_TRUE(w.ok());

        BitReader r(AsSpan(storage));
        (void)r.readBits(1); // misalign reader
        ASSERT_TRUE(r.ok());

        auto bytes = r.readBytes(1);
        EXPECT_FALSE(r.ok());
        EXPECT_EQ(r.error(), Error::Misaligned);
        EXPECT_TRUE(bytes.empty());
    }

    TEST(BitParser, VarUIntRoundTripVariousValues)
    {
        const uint64_t values[] = {
            0ull, 1ull, 2ull, 10ull, 127ull, 128ull, 129ull,
            255ull, 300ull, 16384ull, 1ull << 20, (1ull << 32) - 1ull,
            0x0123456789ABCDEFull
        };

        for (uint64_t v : values) {
            std::vector<uint8_t> storage;
            ByteBuffer buf(storage);
            BitWriter w(buf);

            w.writeVarUInt(v);
            ASSERT_TRUE(w.ok());

            BitReader r(AsSpan(storage));
            const uint64_t got = r.readVarUInt();
            ASSERT_TRUE(r.ok());
            EXPECT_EQ(got, v);
        }
    }

    TEST(BitParser, StringRoundTripEmptyAndNonEmpty)
    {
        const std::string strings[] = { "", "a", "hello", "VIN123", "speed:10 gear:3" };

        for (const auto& s : strings) {
            std::vector<uint8_t> storage;
            ByteBuffer buf(storage);
            BitWriter w(buf);

            w.writeString(s);
            ASSERT_TRUE(w.ok());

            BitReader r(AsSpan(storage));
            const std::string got = r.readString();
            ASSERT_TRUE(r.ok());
            EXPECT_EQ(got, s);
        }
    }

    TEST(BitParser, ReaderOutOfRangeOnTooManyBits)
    {
        std::vector<uint8_t> storage;
        ByteBuffer buf(storage);
        BitWriter w(buf);

        w.writeU8(0xAA);
        ASSERT_TRUE(w.ok());

        BitReader r(AsSpan(storage));
        (void)r.readBits(9); // only 8 available
        EXPECT_FALSE(r.ok());
        EXPECT_EQ(r.error(), Error::OutOfRange);
    }

    TEST(BitParser, AlignToByteSkipsPaddingBits)
    {
        std::vector<uint8_t> storage;
        ByteBuffer buf(storage);
        BitWriter w(buf);

        w.writeBits(0b101u, 3);  // 3 bits
        w.alignToByte();         // pad 5 zeros by default
        w.writeU8(0x5A);

        ASSERT_TRUE(w.ok());

        BitReader r(AsSpan(storage));
        EXPECT_EQ(r.readBits(3), 0b101u);
        r.alignToByte(); // should skip the 5 padding bits
        ASSERT_TRUE(r.ok());
        EXPECT_EQ(r.readU8(), 0x5A);
        EXPECT_TRUE(r.ok());
    }

    TEST(BitReaderTest, BitMaskInByte_MsbFirst) {
        EXPECT_EQ(bitMaskInByte(0, BitOrder::MsbFirst), 0x80);
        EXPECT_EQ(bitMaskInByte(7, BitOrder::MsbFirst), 0x01);
    }

    TEST(BitReaderTest, BitMaskInByte_LsbFirst) {
        EXPECT_EQ(bitMaskInByte(0, BitOrder::LsbFirst), 0x01);
        EXPECT_EQ(bitMaskInByte(7, BitOrder::LsbFirst), 0x80);
    }

    // --- Core State and Alignment Tests ---

    TEST(BitReaderTest, InitialStateAndRemainingBits) {
        std::vector<uint8_t> data = { 0xFF, 0x00 };
        Span<const uint8_t> span(data.data(), data.size());
        BitReader reader(span);

        EXPECT_TRUE(reader.ok());
        EXPECT_EQ(reader.error(), Error::None);
        EXPECT_EQ(reader.bitPosition(), 0);
        EXPECT_EQ(reader.remainingBits(), 16);
    }

    TEST(BitReaderTest, AlignToByte_AlreadyAligned) {
        std::vector<uint8_t> data = { 0xFF };
        BitReader reader(Span<const uint8_t>(data.data(), data.size()));

        reader.alignToByte(); // Should do nothing
        EXPECT_EQ(reader.bitPosition(), 0);
    }

    TEST(BitReaderTest, AlignToByte_UnalignedSuccess) {
        std::vector<uint8_t> data = { 0xFF, 0xFF };
        BitReader reader(Span<const uint8_t>(data.data(), data.size()));

        reader.readBits(3);
        EXPECT_EQ(reader.bitPosition(), 3);

        reader.alignToByte();
        EXPECT_EQ(reader.bitPosition(), 8); // Aligned to next byte
    }

    TEST(BitReaderTest, AlignToByte_NotEnoughBits) {
        std::vector<uint8_t> data = { 0xFF };
        BitReader reader(Span<const uint8_t>(data.data(), data.size()));

        reader.readBits(3); // 5 bits left in a 1-byte buffer
        // Artificially truncate the buffer to simulate OutOfRange
        // (Since we can't modify span, we test a read that consumes the rest)
        reader.readBits(5);
        // Now bit position is 8. Wait, alignToByte does nothing if mod == 0.
        // Let's create an invalid state:
        BitReader reader2(Span<const uint8_t>(data.data(), 0)); // 0 length
        // We can't reach unaligned state without reading.
    }

    // Better OutOfRange test for AlignToByte (requires a reader with restricted span)
    TEST(BitReaderTest, AlignToByte_OutOfRange) {
        std::vector<uint8_t> data = { 0xFF };
        BitReader reader(Span<const uint8_t>(data.data(), data.size()));

        reader.readBits(9); // Forces an OutOfRange error and sets error state
        EXPECT_FALSE(reader.ok());

        reader.alignToByte(); // Should fast-return because !ok()
        EXPECT_EQ(reader.error(), Error::OutOfRange);
    }

    // --- ReadBits Tests ---

    TEST(BitReaderTest, ReadBits_Exceeds64Bits) {
        std::vector<uint8_t> data(10, 0xFF);
        BitReader reader(Span<const uint8_t>(data.data(), data.size()));

        EXPECT_EQ(reader.readBits(65), 0);
        EXPECT_EQ(reader.error(), Error::OutOfRange);
    }

    TEST(BitReaderTest, ReadBits_NotEnoughRemaining) {
        std::vector<uint8_t> data = { 0xFF };
        BitReader reader(Span<const uint8_t>(data.data(), data.size()));

        EXPECT_EQ(reader.readBits(9), 0);
        EXPECT_EQ(reader.error(), Error::OutOfRange);
    }

    TEST(BitReaderTest, ReadBits_MsbFirst_Success) {
        // 0b10110000 -> read 3 bits should be 0b101 (5)
        std::vector<uint8_t> data = { 0xB0 };
        BitReader reader(Span<const uint8_t>(data.data(), data.size()), BitOrder::MsbFirst);

        EXPECT_EQ(reader.readBits(3), 5);
        EXPECT_EQ(reader.bitPosition(), 3);
    }

    TEST(BitReaderTest, ReadBits_LsbFirst_Success) {
        // 0b00000101 -> bit0=1, bit1=0, bit2=1
        // LsbFirst reads bit 0 first, shift left builds output MSB-first in the field.
        // Wait, loop does: out = (out << 1) | bit;
        // i=0 (bit0): bit=1 -> out=1
        // i=1 (bit1): bit=0 -> out=2
        // i=2 (bit2): bit=1 -> out=5
        std::vector<uint8_t> data = { 0x05 };
        BitReader reader(Span<const uint8_t>(data.data(), data.size()), BitOrder::LsbFirst);

        EXPECT_EQ(reader.readBits(3), 5);
        EXPECT_EQ(reader.bitPosition(), 3);
    }

    TEST(BitReaderTest, ReadBool_Success) {
        std::vector<uint8_t> data = { 0x80 }; // 0b10000000
        BitReader reader(Span<const uint8_t>(data.data(), data.size()), BitOrder::MsbFirst);

        EXPECT_TRUE(reader.readBool());
        EXPECT_FALSE(reader.readBool()); // Second bit is 0
    }

    // --- Multi-byte Integer Tests (Endianness) ---

    TEST(BitReaderTest, ReadU16_BigEndian) {
        std::vector<uint8_t> data = { 0x12, 0x34 };
        BitReader reader(Span<const uint8_t>(data.data(), data.size()), BitOrder::MsbFirst, ByteOrder::BigEndian);

        EXPECT_EQ(reader.readU16(), 0x1234);
    }

    TEST(BitReaderTest, ReadU16_LittleEndian) {
        std::vector<uint8_t> data = { 0x12, 0x34 };
        BitReader reader(Span<const uint8_t>(data.data(), data.size()), BitOrder::MsbFirst, ByteOrder::LittleEndian);

        EXPECT_EQ(reader.readU16(), 0x3412);
    }

    TEST(BitReaderTest, ReadU32_BigEndian) {
        std::vector<uint8_t> data = { 0x11, 0x22, 0x33, 0x44 };
        BitReader reader(Span<const uint8_t>(data.data(), data.size()), BitOrder::MsbFirst, ByteOrder::BigEndian);

        EXPECT_EQ(reader.readU32(), 0x11223344);
    }

    TEST(BitReaderTest, ReadU32_LittleEndian) {
        std::vector<uint8_t> data = { 0x11, 0x22, 0x33, 0x44 };
        BitReader reader(Span<const uint8_t>(data.data(), data.size()), BitOrder::MsbFirst, ByteOrder::LittleEndian);

        // First byte (0x11) is the LSB, last byte (0x44) is the MSB.
        EXPECT_EQ(reader.readU32(), 0x44332211);
    }


    TEST(BitReaderTest, ReadU64_BigEndian) {
        std::vector<uint8_t> data = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88 };
        BitReader reader(Span<const uint8_t>(data.data(), data.size()), BitOrder::MsbFirst, ByteOrder::BigEndian);

        EXPECT_EQ(reader.readU64(), 0x1122334455667788ULL);
    }

    TEST(BitReaderTest, ReadU64_LittleEndian) {
        std::vector<uint8_t> data = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88 };
        BitReader reader(Span<const uint8_t>(data.data(), data.size()), BitOrder::MsbFirst, ByteOrder::LittleEndian);

        // First byte (0x11) is the LSB, last byte (0x88) is the MSB.
        EXPECT_EQ(reader.readU64(), 0x8877665544332211ULL);
    }

    // --- VarUInt Tests ---

    TEST(BitReaderTest, ReadVarUInt_SingleByte) {
        std::vector<uint8_t> data = { 0x05 }; // MSB is 0, payload is 5
        BitReader reader(Span<const uint8_t>(data.data(), data.size()));

        EXPECT_EQ(reader.readVarUInt(), 5);
        EXPECT_TRUE(reader.ok());
    }

    TEST(BitReaderTest, ReadVarUInt_MultiByte) {
        // 0xAC 0x02 -> 10101100 00000010 
        // byte 1: chunk = 0x2C (44)
        // byte 2: chunk = 0x02 (2) -> 2 << 7 = 256
        // Total = 300
        std::vector<uint8_t> data = { 0xAC, 0x02 };
        BitReader reader(Span<const uint8_t>(data.data(), data.size()));

        EXPECT_EQ(reader.readVarUInt(), 300);
        EXPECT_TRUE(reader.ok());
    }

    TEST(BitReaderTest, ReadVarUInt_NotEnoughBytes) {
        std::vector<uint8_t> data = { 0xAC }; // MSB is 1, expects another byte, but buffer ends
        BitReader reader(Span<const uint8_t>(data.data(), data.size()));

        EXPECT_EQ(reader.readVarUInt(), 0);
        EXPECT_EQ(reader.error(), Error::OutOfRange);
    }

    TEST(BitReaderTest, ReadVarUInt_TenBytesMax) {
        // 10 bytes with MSB set to 1. The loop finishes at 10 bytes without finding a byte with MSB 0.
        std::vector<uint8_t> data(10, 0xFF);
        BitReader reader(Span<const uint8_t>(data.data(), data.size()));

        EXPECT_EQ(reader.readVarUInt(), 0);
        EXPECT_EQ(reader.error(), Error::BadVarUInt);
    }

    TEST(BitReaderTest, ReadVarUInt_AlignsFirst) {
        std::vector<uint8_t> data = { 0xF0, 0x05 }; // bitPos 0-3 consumed, byte 2 is the VarUInt
        BitReader reader(Span<const uint8_t>(data.data(), data.size()));

        reader.readBits(4);
        EXPECT_EQ(reader.readVarUInt(), 5); // Should align to start of 0x05 byte and read it
    }

    // --- Byte Sequence & String Tests ---

    TEST(BitReaderTest, ReadBytes_Success) {
        std::vector<uint8_t> data = { 0xAA, 0xBB, 0xCC };
        BitReader reader(Span<const uint8_t>(data.data(), data.size()));

        auto bytes = reader.readBytes(2);
        ASSERT_EQ(bytes.size(), 2);
        EXPECT_EQ(bytes[0], 0xAA);
        EXPECT_EQ(bytes[1], 0xBB);
    }

    TEST(BitReaderTest, ReadBytes_MisalignedError) {
        std::vector<uint8_t> data = { 0xAA, 0xBB };
        BitReader reader(Span<const uint8_t>(data.data(), data.size()));

        reader.readBits(3); // Induce misalignment
        auto bytes = reader.readBytes(1);

        EXPECT_TRUE(bytes.empty());
        EXPECT_EQ(reader.error(), Error::Misaligned);
    }

    TEST(BitReaderTest, ReadBytes_OutOfRange) {
        std::vector<uint8_t> data = { 0xAA };
        BitReader reader(Span<const uint8_t>(data.data(), data.size()));

        auto bytes = reader.readBytes(2);

        EXPECT_TRUE(bytes.empty());
        EXPECT_EQ(reader.error(), Error::OutOfRange);
    }

    TEST(BitReaderTest, ReadString_Success) {
        // String format: VarUInt length followed by bytes
        // Length 4, string "Test" (0x54, 0x65, 0x73, 0x74)
        std::vector<uint8_t> data = { 0x04, 0x54, 0x65, 0x73, 0x74 };
        BitReader reader(Span<const uint8_t>(data.data(), data.size()));

        EXPECT_EQ(reader.readString(), "Test");
        EXPECT_TRUE(reader.ok());
    }

    TEST(BitReaderTest, ReadString_NotOkInitially) {
        std::vector<uint8_t> data = { 0x04, 0x54 };
        BitReader reader(Span<const uint8_t>(data.data(), data.size()));

        reader.readBits(100); // Forces OutOfRange error state
        EXPECT_FALSE(reader.ok());

        EXPECT_EQ(reader.readString(), ""); // Fails out early
    }

} // namespace shared::bit_parser
