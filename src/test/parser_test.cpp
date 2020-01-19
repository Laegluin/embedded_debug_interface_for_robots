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

TEST_CASE("parse instruction packet from generic packet", "[parse_instruction_packet]") {
    Parser parser;

    Packet packet{
        DeviceId(0),
        Instruction::Ping,
        Error(),
        FixedByteVector<PACKET_BUF_LEN>(),
    };

    InstructionPacket instruction_packet;

    SECTION("ping packet") {
        uint8_t raw_packet[]{0xff, 0xff, 0xfd, 0x00, 0x01, 0x03, 0x00, 0x01, 0x19, 0x4e};
        Cursor cursor(raw_packet, sizeof(raw_packet));
        auto parse_result = parser.parse(cursor, packet);

        REQUIRE(cursor.remaining_bytes() == 0);
        REQUIRE(parse_result == ParseResult::packet_available());

        auto result = parse_instruction_packet(packet, &instruction_packet);

        REQUIRE(result == InstructionParseResult::Ok);
        REQUIRE(instruction_packet.instruction == Instruction::Ping);
        REQUIRE(instruction_packet.ping.device_id == DeviceId(1));
    }

    SECTION("read packet") {
        uint8_t raw_packet[]{
            0xff, 0xff, 0xfd, 0x00, 0x01, 0x07, 0x00, 0x02, 0x84, 0x00, 0x04, 0x00, 0x1d, 0x15};
        Cursor cursor(raw_packet, sizeof(raw_packet));
        auto parse_result = parser.parse(cursor, packet);

        REQUIRE(cursor.remaining_bytes() == 0);
        REQUIRE(parse_result == ParseResult::packet_available());

        auto result = parse_instruction_packet(packet, &instruction_packet);

        REQUIRE(result == InstructionParseResult::Ok);
        REQUIRE(instruction_packet.instruction == Instruction::Read);
        REQUIRE(instruction_packet.read.device_id == DeviceId(1));
        REQUIRE(instruction_packet.read.start_addr == 0x0084);
        REQUIRE(instruction_packet.read.len == 4);
    }

    SECTION("write packet") {
        uint8_t raw_packet[]{0xff,
                             0xff,
                             0xfd,
                             0x00,
                             0x01,
                             0x09,
                             0x00,
                             0x03,
                             0x74,
                             0x00,
                             0x00,
                             0x02,
                             0x00,
                             0x00,
                             0xca,
                             0x89};

        Cursor cursor(raw_packet, sizeof(raw_packet));
        auto parse_result = parser.parse(cursor, packet);

        REQUIRE(cursor.remaining_bytes() == 0);
        REQUIRE(parse_result == ParseResult::packet_available());

        auto result = parse_instruction_packet(packet, &instruction_packet);

        REQUIRE(result == InstructionParseResult::Ok);
        REQUIRE(instruction_packet.instruction == Instruction::Write);
        REQUIRE(instruction_packet.write.device_id == DeviceId(1));
        REQUIRE(instruction_packet.write.start_addr == 0x0074);
        REQUIRE(instruction_packet.write.data == std::vector<uint8_t>{0x00, 0x02, 0x00, 0x00});
    }

    SECTION("reg write packet") {
        uint8_t raw_packet[]{0xff,
                             0xff,
                             0xfd,
                             0x00,
                             0x01,
                             0x09,
                             0x00,
                             0x04,
                             0x68,
                             0x00,
                             0xc8,
                             0x00,
                             0x00,
                             0x00,
                             0xae,
                             0x8e};

        Cursor cursor(raw_packet, sizeof(raw_packet));
        auto parse_result = parser.parse(cursor, packet);

        REQUIRE(cursor.remaining_bytes() == 0);
        REQUIRE(parse_result == ParseResult::packet_available());

        auto result = parse_instruction_packet(packet, &instruction_packet);

        REQUIRE(result == InstructionParseResult::Ok);
        REQUIRE(instruction_packet.instruction == Instruction::RegWrite);
        REQUIRE(instruction_packet.reg_write.device_id == DeviceId(1));
        REQUIRE(instruction_packet.reg_write.start_addr == 0x0068);
        REQUIRE(instruction_packet.reg_write.data == std::vector<uint8_t>{0xc8, 0x00, 0x00, 0x00});
    }

    SECTION("action packet") {
        uint8_t raw_packet[]{0xff, 0xff, 0xfd, 0x00, 0x01, 0x03, 0x00, 0x05, 0x02, 0xce};
        Cursor cursor(raw_packet, sizeof(raw_packet));
        auto parse_result = parser.parse(cursor, packet);

        REQUIRE(cursor.remaining_bytes() == 0);
        REQUIRE(parse_result == ParseResult::packet_available());

        auto result = parse_instruction_packet(packet, &instruction_packet);

        REQUIRE(result == InstructionParseResult::Ok);
        REQUIRE(instruction_packet.instruction == Instruction::Action);
    }

    SECTION("factory reset packet") {
        uint8_t raw_packet[]{0xff, 0xff, 0xfd, 0x00, 0x01, 0x04, 0x00, 0x06, 0x01, 0xa1, 0xe6};
        Cursor cursor(raw_packet, sizeof(raw_packet));
        auto parse_result = parser.parse(cursor, packet);

        REQUIRE(cursor.remaining_bytes() == 0);
        REQUIRE(parse_result == ParseResult::packet_available());

        auto result = parse_instruction_packet(packet, &instruction_packet);

        REQUIRE(result == InstructionParseResult::Ok);
        REQUIRE(instruction_packet.instruction == Instruction::FactoryReset);
        REQUIRE(instruction_packet.factory_reset.device_id == DeviceId(1));
        REQUIRE(instruction_packet.factory_reset.reset == FactoryReset::ResetAllExceptId);
    }

    SECTION("reboot packet") {
        uint8_t raw_packet[]{0xff, 0xff, 0xfd, 0x00, 0x01, 0x03, 0x00, 0x08, 0x2f, 0x4e};
        Cursor cursor(raw_packet, sizeof(raw_packet));
        auto parse_result = parser.parse(cursor, packet);

        REQUIRE(cursor.remaining_bytes() == 0);
        REQUIRE(parse_result == ParseResult::packet_available());

        auto result = parse_instruction_packet(packet, &instruction_packet);

        REQUIRE(result == InstructionParseResult::Ok);
        REQUIRE(instruction_packet.instruction == Instruction::Reboot);
    }

    SECTION("clear packet") {
        uint8_t raw_packet[]{0xff,
                             0xff,
                             0xfd,
                             0x00,
                             0x01,
                             0x08,
                             0x00,
                             0x10,
                             0x01,
                             0x44,
                             0x58,
                             0x4c,
                             0x22,
                             0xb1,
                             0xdc};

        Cursor cursor(raw_packet, sizeof(raw_packet));
        auto parse_result = parser.parse(cursor, packet);

        REQUIRE(cursor.remaining_bytes() == 0);
        REQUIRE(parse_result == ParseResult::packet_available());

        auto result = parse_instruction_packet(packet, &instruction_packet);

        REQUIRE(result == InstructionParseResult::Ok);
        REQUIRE(instruction_packet.instruction == Instruction::Clear);
    }

    SECTION("sync read") {
        uint8_t raw_packet[]{0xff,
                             0xff,
                             0xfd,
                             0x00,
                             0xfe,
                             0x09,
                             0x00,
                             0x82,
                             0x84,
                             0x00,
                             0x04,
                             0x00,
                             0x01,
                             0x02,
                             0xce,
                             0xfa};

        Cursor cursor(raw_packet, sizeof(raw_packet));
        auto parse_result = parser.parse(cursor, packet);

        REQUIRE(cursor.remaining_bytes() == 0);
        REQUIRE(parse_result == ParseResult::packet_available());

        auto result = parse_instruction_packet(packet, &instruction_packet);

        REQUIRE(result == InstructionParseResult::Ok);
        REQUIRE(instruction_packet.instruction == Instruction::SyncRead);
        REQUIRE(
            instruction_packet.sync_read.devices
            == std::vector<DeviceId>{DeviceId(1), DeviceId(2)});
        REQUIRE(instruction_packet.sync_read.start_addr == 0x0084);
        REQUIRE(instruction_packet.sync_read.len == 4);
    }

    SECTION("sync write") {
        uint8_t raw_packet[]{0xff, 0xff, 0xfd, 0x00, 0xfe, 0x11, 0x00, 0x83,
                             0x74, 0x00, 0x04, 0x00, 0x01, 0x96, 0x00, 0x00,
                             0x00, 0x02, 0xaa, 0x00, 0x00, 0x00, 0x82, 0x87};

        Cursor cursor(raw_packet, sizeof(raw_packet));
        auto parse_result = parser.parse(cursor, packet);

        REQUIRE(cursor.remaining_bytes() == 0);
        REQUIRE(parse_result == ParseResult::packet_available());

        auto result = parse_instruction_packet(packet, &instruction_packet);

        REQUIRE(result == InstructionParseResult::Ok);
        REQUIRE(instruction_packet.instruction == Instruction::SyncWrite);
        REQUIRE(
            instruction_packet.sync_write.devices
            == std::vector<DeviceId>{DeviceId(1), DeviceId(2)});
        REQUIRE(instruction_packet.sync_write.start_addr == 0x0074);
        REQUIRE(instruction_packet.sync_write.len == 4);
        REQUIRE(
            instruction_packet.sync_write.data
            == std::vector<uint8_t>{0x96, 0x00, 0x00, 0x00, 0xaa, 0x00, 0x00, 0x00});
    }

    SECTION("bulk read") {
        uint8_t raw_packet[]{0xff, 0xff, 0xfd, 0x00, 0xfe, 0x0d, 0x00, 0x92, 0x01, 0x90,
                             0x00, 0x02, 0x00, 0x02, 0x92, 0x00, 0x01, 0x00, 0x1a, 0x05};

        Cursor cursor(raw_packet, sizeof(raw_packet));
        auto parse_result = parser.parse(cursor, packet);

        REQUIRE(cursor.remaining_bytes() == 0);
        REQUIRE(parse_result == ParseResult::packet_available());

        auto result = parse_instruction_packet(packet, &instruction_packet);

        REQUIRE(result == InstructionParseResult::Ok);
        REQUIRE(instruction_packet.instruction == Instruction::BulkRead);
        REQUIRE(instruction_packet.bulk_read.reads.size() == 2);

        REQUIRE(instruction_packet.bulk_read.reads[0].device_id == DeviceId(1));
        REQUIRE(instruction_packet.bulk_read.reads[0].start_addr == 0x0090);
        REQUIRE(instruction_packet.bulk_read.reads[0].len == 2);

        REQUIRE(instruction_packet.bulk_read.reads[1].device_id == DeviceId(2));
        REQUIRE(instruction_packet.bulk_read.reads[1].start_addr == 0x0092);
        REQUIRE(instruction_packet.bulk_read.reads[1].len == 1);
    }

    SECTION("bulk write") {
        uint8_t raw_packet[]{0xff, 0xff, 0xfd, 0x00, 0xfe, 0x10, 0x00, 0x93, 0x01, 0x20, 0x00, 0x02,
                             0x00, 0xa0, 0x00, 0x02, 0x1f, 0x00, 0x01, 0x00, 0x50, 0xb7, 0x68};

        Cursor cursor(raw_packet, sizeof(raw_packet));
        auto parse_result = parser.parse(cursor, packet);

        REQUIRE(cursor.remaining_bytes() == 0);
        REQUIRE(parse_result == ParseResult::packet_available());

        auto result = parse_instruction_packet(packet, &instruction_packet);

        REQUIRE(result == InstructionParseResult::Ok);
        REQUIRE(instruction_packet.instruction == Instruction::BulkWrite);
        REQUIRE(instruction_packet.bulk_write.writes.size() == 2);

        REQUIRE(instruction_packet.bulk_write.writes[0].device_id == DeviceId(1));
        REQUIRE(instruction_packet.bulk_write.writes[0].start_addr == 0x0020);
        REQUIRE(instruction_packet.bulk_write.writes[0].data.size() == 2);
        REQUIRE(instruction_packet.bulk_write.writes[0].data == std::vector<uint8_t>{0xa0, 0x00});

        REQUIRE(instruction_packet.bulk_write.writes[1].device_id == DeviceId(2));
        REQUIRE(instruction_packet.bulk_write.writes[1].start_addr == 0x001f);
        REQUIRE(instruction_packet.bulk_write.writes[1].data.size() == 1);
        REQUIRE(instruction_packet.bulk_write.writes[1].data == std::vector<uint8_t>{0x50});
    }
}
