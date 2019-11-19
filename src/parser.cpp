#include "parser.h"
#include <array>

const std::array<uint8_t, 3> HEADER = {0xff, 0xff, 0xfd};
const uint8_t STUFFING_BYTE = 0xfd;
const uint8_t HEADER_TRAILING_BYTE = 0x00;

bool Receiver::wait_for_header(Cursor& cursor) {
    uint8_t current_byte;

    while (cursor.read(&current_byte, 1) != 0) {
        auto is_header = current_byte == HEADER_TRAILING_BYTE && this->last_bytes == HEADER;
        this->push_last_byte(current_byte);

        if (is_header) {
            // reset crc for new packet, then add the header
            this->reset_crc();
            this->update_crc(HEADER[0]);
            this->update_crc(HEADER[1]);
            this->update_crc(HEADER[2]);
            this->update_crc(HEADER_TRAILING_BYTE);

            return true;
        }
    }

    return false;
}

size_t Receiver::read(Cursor& cursor, uint8_t* dst, size_t num_bytes) {
    size_t bytes_read = 0;
    uint8_t current_byte;

    while (bytes_read < num_bytes && cursor.read(&current_byte, 1) != 0) {
        if (!this->is_stuffing_byte(current_byte)) {
            dst[bytes_read] = current_byte;
            bytes_read++;
        }

        this->push_last_byte(current_byte);
        this->update_crc(current_byte);
    }

    return bytes_read;
}

size_t Receiver::read_raw_num_bytes(
    Cursor& cursor,
    uint8_t* dst,
    size_t* dst_len,
    size_t raw_num_bytes) {
    *dst_len = 0;
    size_t raw_bytes_read = 0;
    uint8_t current_byte;

    while (raw_bytes_read < raw_num_bytes && cursor.read(&current_byte, 1) != 0) {
        if (!this->is_stuffing_byte(current_byte)) {
            dst[*dst_len] = current_byte;
            (*dst_len)++;
        }

        this->push_last_byte(current_byte);
        this->update_crc(current_byte);
        raw_bytes_read++;
    }

    return raw_bytes_read;
}

size_t Receiver::read_raw(Cursor& cursor, uint8_t* dst, size_t num_bytes) {
    size_t bytes_read = 0;
    uint8_t current_byte;

    while (bytes_read < num_bytes && cursor.read(&current_byte, 1) != 0) {
        this->push_last_byte(current_byte);
        dst[bytes_read] = current_byte;
        bytes_read++;
    }

    return bytes_read;
}

bool Receiver::is_stuffing_byte(uint8_t byte) const {
    return byte == STUFFING_BYTE && this->last_bytes == HEADER;
}

void Receiver::push_last_byte(uint8_t byte) {
    this->last_bytes[0] = this->last_bytes[1];
    this->last_bytes[1] = this->last_bytes[2];
    this->last_bytes[2] = byte;
}

uint16_t Receiver::current_crc() const {
    return this->crc;
}

void Receiver::reset_crc() {
    this->crc = 0;
}

void Receiver::update_crc(uint8_t byte) {
    const uint16_t CRC_TABLE[256] = {
        0x0000, 0x8005, 0x800f, 0x000a, 0x801b, 0x001e, 0x0014, 0x8011, 0x8033, 0x0036, 0x003c,
        0x8039, 0x0028, 0x802d, 0x8027, 0x0022, 0x8063, 0x0066, 0x006c, 0x8069, 0x0078, 0x807d,
        0x8077, 0x0072, 0x0050, 0x8055, 0x805f, 0x005a, 0x804b, 0x004e, 0x0044, 0x8041, 0x80c3,
        0x00c6, 0x00cc, 0x80c9, 0x00d8, 0x80dd, 0x80d7, 0x00d2, 0x00f0, 0x80f5, 0x80ff, 0x00fa,
        0x80eb, 0x00ee, 0x00e4, 0x80e1, 0x00a0, 0x80a5, 0x80af, 0x00aa, 0x80bb, 0x00be, 0x00b4,
        0x80b1, 0x8093, 0x0096, 0x009c, 0x8099, 0x0088, 0x808d, 0x8087, 0x0082, 0x8183, 0x0186,
        0x018c, 0x8189, 0x0198, 0x819d, 0x8197, 0x0192, 0x01b0, 0x81b5, 0x81bf, 0x01ba, 0x81ab,
        0x01ae, 0x01a4, 0x81a1, 0x01e0, 0x81e5, 0x81ef, 0x01ea, 0x81fb, 0x01fe, 0x01f4, 0x81f1,
        0x81d3, 0x01d6, 0x01dc, 0x81d9, 0x01c8, 0x81cd, 0x81c7, 0x01c2, 0x0140, 0x8145, 0x814f,
        0x014a, 0x815b, 0x015e, 0x0154, 0x8151, 0x8173, 0x0176, 0x017c, 0x8179, 0x0168, 0x816d,
        0x8167, 0x0162, 0x8123, 0x0126, 0x012c, 0x8129, 0x0138, 0x813d, 0x8137, 0x0132, 0x0110,
        0x8115, 0x811f, 0x011a, 0x810b, 0x010e, 0x0104, 0x8101, 0x8303, 0x0306, 0x030c, 0x8309,
        0x0318, 0x831d, 0x8317, 0x0312, 0x0330, 0x8335, 0x833f, 0x033a, 0x832b, 0x032e, 0x0324,
        0x8321, 0x0360, 0x8365, 0x836f, 0x036a, 0x837b, 0x037e, 0x0374, 0x8371, 0x8353, 0x0356,
        0x035c, 0x8359, 0x0348, 0x834d, 0x8347, 0x0342, 0x03c0, 0x83c5, 0x83cf, 0x03ca, 0x83db,
        0x03de, 0x03d4, 0x83d1, 0x83f3, 0x03f6, 0x03fc, 0x83f9, 0x03e8, 0x83ed, 0x83e7, 0x03e2,
        0x83a3, 0x03a6, 0x03ac, 0x83a9, 0x03b8, 0x83bd, 0x83b7, 0x03b2, 0x0390, 0x8395, 0x839f,
        0x039a, 0x838b, 0x038e, 0x0384, 0x8381, 0x0280, 0x8285, 0x828f, 0x028a, 0x829b, 0x029e,
        0x0294, 0x8291, 0x82b3, 0x02b6, 0x02bc, 0x82b9, 0x02a8, 0x82ad, 0x82a7, 0x02a2, 0x82e3,
        0x02e6, 0x02ec, 0x82e9, 0x02f8, 0x82fd, 0x82f7, 0x02f2, 0x02d0, 0x82d5, 0x82df, 0x02da,
        0x82cb, 0x02ce, 0x02c4, 0x82c1, 0x8243, 0x0246, 0x024c, 0x8249, 0x0258, 0x825d, 0x8257,
        0x0252, 0x0270, 0x8275, 0x827f, 0x027a, 0x826b, 0x026e, 0x0264, 0x8261, 0x0220, 0x8225,
        0x822f, 0x022a, 0x823b, 0x023e, 0x0234, 0x8231, 0x8213, 0x0216, 0x021c, 0x8219, 0x0208,
        0x820d, 0x8207, 0x0202};

    size_t index = ((uint16_t)(this->crc >> 8) ^ byte) & 0xff;
    this->crc = (this->crc << 8) ^ CRC_TABLE[index];
}

Option<Instruction> try_instruction_from_byte(uint8_t instr) {
    auto maybe_instr = Instruction(instr);

    switch (maybe_instr) {
        case Instruction::Ping:
        case Instruction::Read:
        case Instruction::Write:
        case Instruction::RegWrite:
        case Instruction::Action:
        case Instruction::FactoryReset:
        case Instruction::Reboot:
        case Instruction::Clear:
        case Instruction::Status:
        case Instruction::SyncRead:
        case Instruction::SyncWrite:
        case Instruction::BulkRead:
        case Instruction::BulkWrite:
            return Option<Instruction>(maybe_instr);

        default:
            return Option<Instruction>();
    }
}

uint16_t uint16_le(uint8_t bytes[2]) {
    return (bytes[1] << 8) + bytes[0];
}

ParseResult Parser::parse(Cursor& cursor, Packet& packet) {
    // fallthrough is intended here; we only need the switch to resume when we reenter after getting
    // new data
    switch (this->current_state) {
        case ParserState::Header: {
            // clear the data so we can overwrite it later;
            // this is fine since the state of the packet is undefined until
            // we return a result indicating that it is ready
            packet.data.clear();

            if (!this->receiver.wait_for_header(cursor)) {
                return ParseResult::need_more_data();
            }

            this->buf.clear();
            this->current_state = ParserState::CommonFields;
        }
        // fallthrough
        case ParserState::CommonFields: {
            const size_t COMMON_FIELDS_LEN = 4;
            auto needed_bytes = COMMON_FIELDS_LEN - this->buf.size();

            uint8_t* dst = this->buf.data() + this->buf.size();
            this->buf.extend_by(needed_bytes);
            auto bytes_read = this->receiver.read(cursor, dst, needed_bytes);
            this->buf.shrink_by(needed_bytes - bytes_read);

            if (this->buf.size() < COMMON_FIELDS_LEN) {
                return ParseResult::need_more_data();
            }

            packet.device_id = DeviceId(this->buf[0]);
            packet.instruction = Instruction(this->buf[3]);

            // byte stuffing can be ignored for the subtractions: the checksum is explicitly not
            // part of the stuffing range and the allowed values for the instruction guarantee that
            // no stuffing can happen for it or the (possibly) following error

            // clang-format off
            this->raw_remaining_data_len = uint16_le(buf.data() + 1) 
                - 1                                                     // instruction field
                - (packet.instruction == Instruction::Status)           // error field (only present on status packets)
                - 2;                                                    // crc checksum
            // clang-format on

            this->buf.clear();
            this->current_state = ParserState::ErrorField;
        }
        // fallthrough
        case ParserState::ErrorField: {
            if (packet.instruction == Instruction::Status) {
                uint8_t error;
                if (this->receiver.read(cursor, &error, 1) == 0) {
                    return ParseResult::need_more_data();
                }

                packet.error = Error(error);
            }

            this->current_state = ParserState::Data;
        }
        // fallthrough
        case ParserState::Data: {
            uint8_t* dst = packet.data.data() + packet.data.size();

            // since there may be stuffing, this length is actually only an upper bound on the
            // number of bytes; it should be close enough though
            if (!packet.data.try_extend_by(this->raw_remaining_data_len)) {
                this->current_state = ParserState::Header;
                return ParseResult(ParseError::BufferOverflow);
            }

            size_t bytes_read;
            auto raw_bytes_read = this->receiver.read_raw_num_bytes(
                cursor, dst, &bytes_read, this->raw_remaining_data_len);

            packet.data.shrink_by(this->raw_remaining_data_len - bytes_read);
            this->raw_remaining_data_len -= raw_bytes_read;

            if (this->raw_remaining_data_len > 0) {
                return ParseResult::need_more_data();
            }

            this->buf.clear();
            this->current_state = ParserState::Checksum;
        }
        // fallthrough
        case ParserState::Checksum: {
            auto needed_bytes = 2 - this->buf.size();

            uint8_t* dst = this->buf.data() + this->buf.size();
            this->buf.extend_by(needed_bytes);
            auto bytes_read = this->receiver.read_raw(cursor, dst, needed_bytes);
            this->buf.shrink_by(needed_bytes - bytes_read);

            if (this->buf.size() < 2) {
                return ParseResult::need_more_data();
            }

            auto checksum = uint16_le(this->buf.data());
            this->buf.clear();
            this->current_state = ParserState::Header;

            if (checksum == this->receiver.current_crc()) {
                return ParseResult::packet_available();
            } else {
                return ParseResult(ParseError::MismatchedChecksum);
            }
        }
        default:
            std::abort();
    }
}

#ifdef TEST
#include "catch2/catch.hpp"

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

#endif
