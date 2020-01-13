#include "parser.h"
#include <catch2/catch.hpp>

TEST_CASE("parse packets", "[Parser]") {
    Parser parser;
    Packet packet{
        DeviceId(0),
        Instruction::Ping,
        Error(),
        FixedByteVector<PACKET_BUF_LEN>(),
    };

    SECTION("ping packet") {
        uint8_t raw_packet[]{0xff, 0xff, 0xfd, 0x00, 0x01, 0x03, 0x00, 0x01, 0x19, 0x4e};
        Cursor cursor(raw_packet, sizeof(raw_packet));
        auto result = parser.parse(cursor, packet);

        REQUIRE(cursor.remaining_bytes() == 0);
        REQUIRE(result == ParseResult::packet_available());
        REQUIRE(packet.device_id == DeviceId(1));
        REQUIRE(packet.instruction == Instruction::Ping);
        REQUIRE(packet.error == Error());
        REQUIRE(packet.data == FixedByteVector<0>());
    }

    SECTION("read packet") {
        uint8_t raw_packet[]{
            0xff, 0xff, 0xfd, 0x00, 0x01, 0x07, 0x00, 0x02, 0x84, 0x00, 0x04, 0x00, 0x1d, 0x15};
        Cursor cursor(raw_packet, sizeof(raw_packet));
        auto result = parser.parse(cursor, packet);

        REQUIRE(cursor.remaining_bytes() == 0);
        REQUIRE(result == ParseResult::packet_available());
        REQUIRE(packet.device_id == DeviceId(1));
        REQUIRE(packet.instruction == Instruction::Read);
        REQUIRE(packet.error == Error());
        REQUIRE(packet.data == FixedByteVector<4>{0x84, 0x00, 0x04, 0x00});
    }

    SECTION("status packet") {
        uint8_t raw_packet[]{0xff,
                             0xff,
                             0xfd,
                             0x00,
                             0x01,
                             0x08,
                             0x00,
                             0x55,
                             0x00,
                             0xa6,
                             0x00,
                             0x00,
                             0x00,
                             0x8c,
                             0xc0};

        Cursor cursor(raw_packet, sizeof(raw_packet));
        auto result = parser.parse(cursor, packet);

        REQUIRE(cursor.remaining_bytes() == 0);
        REQUIRE(result == ParseResult::packet_available());
        REQUIRE(packet.device_id == DeviceId(1));
        REQUIRE(packet.instruction == Instruction::Status);
        REQUIRE(packet.error == Error());
        REQUIRE(packet.data == FixedByteVector<4>{0xa6, 0x00, 0x00, 0x00});
    }

    SECTION("packet with preceding garbage") {
        uint8_t raw_packet[]{0x34,
                             0x12,
                             0xaa,
                             0x5a,
                             0x44,
                             0xff,
                             0xff,
                             0xfd,
                             0x00,
                             0x01,
                             0x07,
                             0x00,
                             0x02,
                             0x84,
                             0x00,
                             0x04,
                             0x00,
                             0x1d,
                             0x15};

        Cursor cursor(raw_packet, sizeof(raw_packet));
        auto result = parser.parse(cursor, packet);

        REQUIRE(cursor.remaining_bytes() == 0);
        REQUIRE(result == ParseResult::packet_available());
        REQUIRE(packet.device_id == DeviceId(1));
        REQUIRE(packet.instruction == Instruction::Read);
        REQUIRE(packet.error == Error());
        REQUIRE(packet.data == FixedByteVector<4>{0x84, 0x00, 0x04, 0x00});
    }

    SECTION("packet with trailing garbage") {
        uint8_t raw_packet[]{
            0xff, 0xff, 0xfd, 0x00, 0x01, 0x07, 0x00, 0x02, 0x84, 0x00,
            0x04, 0x00, 0x1d, 0x15, 0x34, 0x12, 0xaa, 0x5a, 0x44,
        };

        Cursor cursor(raw_packet, sizeof(raw_packet));
        auto result = parser.parse(cursor, packet);

        REQUIRE(cursor.remaining_bytes() == 5);
        REQUIRE(result == ParseResult::packet_available());
        REQUIRE(packet.device_id == DeviceId(1));
        REQUIRE(packet.instruction == Instruction::Read);
        REQUIRE(packet.error == Error());
        REQUIRE(packet.data == FixedByteVector<4>{0x84, 0x00, 0x04, 0x00});

        result = parser.parse(cursor, packet);
        REQUIRE(cursor.remaining_bytes() == 0);
        REQUIRE(result == ParseResult::need_more_data());
    }

    SECTION("packet with stuffing bytes") {
        uint8_t raw_packet[]{
            0xff, 0xff, 0xfd, 0x00, 0x03, 0x07, 0x00, 0x02, 0xff, 0xff, 0xfd, 0xfd, 0x0b, 0x71};
        Cursor cursor(raw_packet, sizeof(raw_packet));
        auto result = parser.parse(cursor, packet);

        REQUIRE(cursor.remaining_bytes() == 0);
        REQUIRE(result == ParseResult::packet_available());
        REQUIRE(packet.device_id == DeviceId(3));
        REQUIRE(packet.instruction == Instruction::Read);
        REQUIRE(packet.error == Error());
        REQUIRE(packet.data == FixedByteVector<3>{0xff, 0xff, 0xfd});
    }

    SECTION("packet split over two buffers") {
        uint8_t part1[]{0xff, 0xff, 0xfd, 0x00, 0x01, 0x07, 0x00};
        uint8_t part2[]{0x02, 0x84, 0x00, 0x04, 0x00, 0x1d, 0x15};

        Cursor cursor(part1, sizeof(part1));
        auto result = parser.parse(cursor, packet);

        REQUIRE(cursor.remaining_bytes() == 0);
        REQUIRE(result == ParseResult::need_more_data());

        cursor = Cursor(part2, sizeof(part2));
        result = parser.parse(cursor, packet);

        REQUIRE(cursor.remaining_bytes() == 0);
        REQUIRE(result == ParseResult::packet_available());
        REQUIRE(packet.device_id == DeviceId(1));
        REQUIRE(packet.instruction == Instruction::Read);
        REQUIRE(packet.error == Error());
        REQUIRE(packet.data == FixedByteVector<4>{0x84, 0x00, 0x04, 0x00});
    }

    SECTION("two consecutive packets") {
        uint8_t raw_packet[]{0xff, 0xff, 0xfd, 0x00, 0x03, 0x07, 0x00, 0x02, 0xff, 0xff,
                             0xfd, 0xfd, 0x0b, 0x71, 0xff, 0xff, 0xfd, 0x00, 0x01, 0x09,
                             0x00, 0x03, 0x74, 0x00, 0x00, 0x02, 0x00, 0x00, 0xca, 0x89};

        Cursor cursor(raw_packet, sizeof(raw_packet));
        auto result = parser.parse(cursor, packet);

        REQUIRE(cursor.remaining_bytes() == 16);
        REQUIRE(result == ParseResult::packet_available());
        REQUIRE(packet.device_id == DeviceId(3));
        REQUIRE(packet.instruction == Instruction::Read);
        REQUIRE(packet.error == Error());
        REQUIRE(packet.data == FixedByteVector<3>{0xff, 0xff, 0xfd});

        result = parser.parse(cursor, packet);

        REQUIRE(cursor.remaining_bytes() == 0);
        REQUIRE(result == ParseResult::packet_available());
        REQUIRE(packet.device_id == DeviceId(1));
        REQUIRE(packet.instruction == Instruction::Write);
        REQUIRE(packet.error == Error());
        REQUIRE(packet.data == FixedByteVector<6>{0x74, 0x00, 0x00, 0x02, 0x00, 0x00});
    }

    SECTION("two packets with garbage inbetween") {
        uint8_t raw_packet[]{0xff, 0xff, 0xfd, 0x00, 0x03, 0x07, 0x00, 0x02, 0xff, 0xff, 0xfd,
                             0xfd, 0x0b, 0x71, 0x75, 0xdf, 0xa4, 0xff, 0xff, 0xfd, 0x00, 0x01,
                             0x09, 0x00, 0x03, 0x74, 0x00, 0x00, 0x02, 0x00, 0x00, 0xca, 0x89};

        Cursor cursor(raw_packet, sizeof(raw_packet));
        auto result = parser.parse(cursor, packet);

        REQUIRE(cursor.remaining_bytes() == 19);
        REQUIRE(result == ParseResult::packet_available());
        REQUIRE(packet.device_id == DeviceId(3));
        REQUIRE(packet.instruction == Instruction::Read);
        REQUIRE(packet.error == Error());
        REQUIRE(packet.data == FixedByteVector<3>{0xff, 0xff, 0xfd});

        result = parser.parse(cursor, packet);

        REQUIRE(cursor.remaining_bytes() == 0);
        REQUIRE(result == ParseResult::packet_available());
        REQUIRE(packet.device_id == DeviceId(1));
        REQUIRE(packet.instruction == Instruction::Write);
        REQUIRE(packet.error == Error());
        REQUIRE(packet.data == FixedByteVector<6>{0x74, 0x00, 0x00, 0x02, 0x00, 0x00});
    }
};

TEST_CASE("parse invalid packets", "[Parser]") {
    Parser parser;
    Packet packet{
        DeviceId(0),
        Instruction::Ping,
        Error(),
        FixedByteVector<PACKET_BUF_LEN>(),
    };

    SECTION("buffer overflow") {
        uint8_t raw_packet[]{
            0xff, 0xff, 0xfd, 0x00, 0x01, 0xff, 0xff, 0x02, 0x84, 0x00, 0x04, 0x00, 0x1d, 0x15};
        Cursor cursor(raw_packet, sizeof(raw_packet));
        auto result = parser.parse(cursor, packet);

        REQUIRE(cursor.remaining_bytes() == 6);
        REQUIRE(result == ParseResult(ParseError::BufferOverflow));
    }

    SECTION("invalid checksum") {
        uint8_t raw_packet[]{
            0xff, 0xff, 0xfd, 0x00, 0x01, 0x07, 0x00, 0x02, 0x84, 0x00, 0x04, 0x00, 0x11, 0x15};
        Cursor cursor(raw_packet, sizeof(raw_packet));
        auto result = parser.parse(cursor, packet);

        REQUIRE(cursor.remaining_bytes() == 0);
        REQUIRE(result == ParseResult(ParseError::MismatchedChecksum));
    }

    SECTION("successful parse after error") {
        uint8_t raw_packet[]{0xff, 0xff, 0xfd, 0x00, 0x01, 0xff, 0xff, 0x02, 0x84, 0x00,
                             0x04, 0x00, 0x1d, 0x15, 0xff, 0xff, 0xfd, 0x00, 0x01, 0x07,
                             0x00, 0x02, 0x84, 0x00, 0x04, 0x00, 0x1d, 0x15};

        Cursor cursor(raw_packet, sizeof(raw_packet));
        auto result = parser.parse(cursor, packet);

        REQUIRE(cursor.remaining_bytes() == 20);
        REQUIRE(result == ParseResult(ParseError::BufferOverflow));

        result = parser.parse(cursor, packet);

        REQUIRE(cursor.remaining_bytes() == 0);
        REQUIRE(result == ParseResult::packet_available());
        REQUIRE(packet.device_id == DeviceId(1));
        REQUIRE(packet.instruction == Instruction::Read);
        REQUIRE(packet.error == Error());
        REQUIRE(packet.data == FixedByteVector<4>{0x84, 0x00, 0x04, 0x00});
    }

    // TODO: detect header and start parsing a new packet instead
    SECTION("allow unescaped header with reserved byte in data") {
        uint8_t raw_packet[]{
            0xff, 0xff, 0xfd, 0x00, 0x01, 0x07, 0x00, 0x02, 0xff, 0xff, 0xfd, 0x00, 0x0a, 0xd3};
        Cursor cursor(raw_packet, sizeof(raw_packet));
        auto result = parser.parse(cursor, packet);

        REQUIRE(cursor.remaining_bytes() == 0);
        REQUIRE(result == ParseResult::packet_available());
        REQUIRE(packet.device_id == DeviceId(1));
        REQUIRE(packet.instruction == Instruction::Read);
        REQUIRE(packet.error == Error());
        REQUIRE(packet.data == FixedByteVector<4>{0xff, 0xff, 0xfd, 0x00});
    }

    // this is technically not allowed by the spec but since it would not be the start of a packet
    // either, there's no real reason not to ignore it
    SECTION("allow unescaped header in data") {
        uint8_t raw_packet[]{
            0xff, 0xff, 0xfd, 0x00, 0x01, 0x07, 0x00, 0x02, 0xff, 0xff, 0xfd, 0x30, 0xaa, 0xd3};
        Cursor cursor(raw_packet, sizeof(raw_packet));
        auto result = parser.parse(cursor, packet);

        REQUIRE(cursor.remaining_bytes() == 0);
        REQUIRE(result == ParseResult::packet_available());
        REQUIRE(packet.device_id == DeviceId(1));
        REQUIRE(packet.instruction == Instruction::Read);
        REQUIRE(packet.error == Error());
        REQUIRE(packet.data == FixedByteVector<4>{0xff, 0xff, 0xfd, 0x30});
    }
}
