// tests/test_bitparser.cpp
#include <gtest/gtest.h>

#include "shared/bit_parser/BitReader.h"
#include "shared/bit_parser/ByteBuffer.h"
#include "shared/bit_parser/BitWriter.h"

#include <cstdint>
#include <string>
#include <vector>

namespace shared::bit_parser
{

    static Span<const uint8_t> AsSpan(const ByteBuffer& b) {
        return Span<const uint8_t>(b.data(), b.size());
    }

    TEST(BitParser, EmptyBufferReadShouldFailOutOfRange) {
        std::vector<uint8_t> empty;
        BitReader r(Span<const uint8_t>(empty.data(), empty.size()));
        EXPECT_FALSE(r.ok());                 // r might still be "ok" until a read
        (void)r.readBits(1);
        EXPECT_FALSE(r.ok());
        EXPECT_EQ(r.error(), Error::OutOfRange);
    }

    TEST(BitParser, WriteAndReadSingleBitsAcrossByteBoundary) {
        ByteBuffer buf;
        BitWriter w(buf);

        // 9 bits: 101010101
        w.writeBits(0b101010101ull, 9);
        ASSERT_TRUE(w.ok());

        BitReader r(AsSpan(buf));
        const auto v = r.readBits(9);
        ASSERT_TRUE(r.ok());
        EXPECT_EQ(v, 0b101010101ull);
    }

    //TEST(BitParser, WriteAndReadFixedWidthIntegersBigEndian) {
    //    ByteBuffer buf;
    //    BitWriter w(buf, BitOrder::MsbFirst, ByteOrder::BigEndian);

    //    w.writeU8(0xAB);
    //    w.writeU16(0x1234);
    //    w.writeU32(0x89ABCDEFu);
    //    w.writeU64(0x0123456789ABCDEFull);

    //    ASSERT_TRUE(w.ok());

    //    BitReader r(AsSpan(buf), BitOrder::MsbFirst, ByteOrder::BigEndian);
    //    EXPECT_EQ(r.readU8(), 0xAB);
    //    EXPECT_EQ(r.readU16(), 0x1234);
    //    EXPECT_EQ(r.readU32(), 0x89ABCDEFu);
    //    EXPECT_EQ(r.readU64(), 0x0123456789ABCDEFull);
    //    EXPECT_TRUE(r.ok());
    //}

    //TEST(BitParser, ArbitraryFieldsAndAlignmentThenString) {
    //    ByteBuffer buf;
    //    BitWriter w(buf);

    //    // Example: msgId (16), speed (12), gear (4) => total 32 bits (already aligned)
    //    w.writeU16(0x1001);
    //    w.writeBits(10, 12);
    //    w.writeBits(3, 4);

    //    // Now write a string (writer aligns before varint internally)
    //    w.writeString("VIN123");
    //    ASSERT_TRUE(w.ok());

    //    BitReader r(AsSpan(buf));
    //    EXPECT_EQ(r.readU16(), 0x1001);
    //    EXPECT_EQ(r.readBits(12), 10u);
    //    EXPECT_EQ(r.readBits(4), 3u);
    //    EXPECT_EQ(r.readString(), "VIN123");
    //    EXPECT_TRUE(r.ok());
    //}

    //TEST(BitParser, MisalignedWriteBytesShouldFail) {
    //    ByteBuffer buf;
    //    BitWriter w(buf);

    //    w.writeBits(1, 1); // now misaligned (bitPos % 8 != 0)
    //    ASSERT_TRUE(w.ok());

    //    const uint8_t payload[2] = { 0xDE, 0xAD };
    //    w.writeBytes(Span<const uint8_t>(payload, 2));

    //    EXPECT_FALSE(w.ok());
    //    EXPECT_EQ(w.error(), Error::Misaligned);
    //}

    //TEST(BitParser, MisalignedReadBytesShouldFail) {
    //    ByteBuffer buf;
    //    BitWriter w(buf);

    //    w.writeBits(1, 1);
    //    // write a byte after aligning so buffer contains data (doesn't matter)
    //    w.alignToByte();
    //    w.writeU8(0xFF);
    //    ASSERT_TRUE(w.ok());

    //    BitReader r(AsSpan(buf));
    //    (void)r.readBits(1); // misalign reader
    //    ASSERT_TRUE(r.ok());

    //    auto bytes = r.readBytes(1);
    //    EXPECT_FALSE(r.ok());
    //    EXPECT_EQ(r.error(), Error::Misaligned);
    //    EXPECT_TRUE(bytes.empty());
    //}

    //TEST(BitParser, VarUIntRoundTripVariousValues) {
    //    const uint64_t values[] = {
    //        0ull, 1ull, 2ull, 10ull, 127ull, 128ull, 129ull,
    //        255ull, 300ull, 16384ull, 1ull << 20, (1ull << 32) - 1ull,
    //        0x0123456789ABCDEFull
    //    };

    //    for (uint64_t v : values) {
    //        ByteBuffer buf;
    //        BitWriter w(buf);
    //        w.writeVarUInt(v);
    //        ASSERT_TRUE(w.ok());

    //        BitReader r(AsSpan(buf));
    //        const uint64_t got = r.readVarUInt();
    //        ASSERT_TRUE(r.ok());
    //        EXPECT_EQ(got, v);
    //    }
    //}

    //TEST(BitParser, StringRoundTripEmptyAndNonEmpty) {
    //    const std::string strings[] = { "", "a", "hello", "VIN123", "speed:10 gear:3" };

    //    for (const auto& s : strings) {
    //        ByteBuffer buf;
    //        BitWriter w(buf);
    //        w.writeString(s);
    //        ASSERT_TRUE(w.ok());

    //        BitReader r(AsSpan(buf));
    //        const std::string got = r.readString();
    //        ASSERT_TRUE(r.ok());
    //        EXPECT_EQ(got, s);
    //    }
    //}

    //TEST(BitParser, ReaderOutOfRangeOnTooManyBits) {
    //    ByteBuffer buf;
    //    BitWriter w(buf);
    //    w.writeU8(0xAA);
    //    ASSERT_TRUE(w.ok());

    //    BitReader r(AsSpan(buf));
    //    (void)r.readBits(9); // only 8 available
    //    EXPECT_FALSE(r.ok());
    //    EXPECT_EQ(r.error(), Error::OutOfRange);
    //}

    //TEST(BitParser, AlignToByteSkipsPaddingBits) {
    //    ByteBuffer buf;
    //    BitWriter w(buf);

    //    w.writeBits(0b101u, 3);  // 3 bits
    //    w.alignToByte();         // pad 5 zeros by default
    //    w.writeU8(0x5A);

    //    ASSERT_TRUE(w.ok());

    //    BitReader r(AsSpan(buf));
    //    EXPECT_EQ(r.readBits(3), 0b101u);
    //    r.alignToByte(); // should skip the 5 padding bits
    //    ASSERT_TRUE(r.ok());
    //    EXPECT_EQ(r.readU8(), 0x5A);
    //    EXPECT_TRUE(r.ok());
    //}
}