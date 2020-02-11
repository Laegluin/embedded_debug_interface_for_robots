#include "parser.h"
#include "endian_convert.h"
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

ParseResult Parser::parse(Cursor& cursor, Packet* packet) {
    // fallthrough is intended here; we only need the switch to resume when we reenter after getting
    // new data
    switch (this->current_state) {
        case ParserState::Header: {
            if (!this->receiver.wait_for_header(cursor)) {
                return ParseResult::NeedMoreData;
            }

            this->buf_len = 0;
            this->current_state = ParserState::CommonFields;
        }
        // fallthrough
        case ParserState::CommonFields: {
            auto needed_bytes = 4 - this->buf_len;

            uint8_t* dst = this->buf + this->buf_len;
            auto bytes_read = this->receiver.read(cursor, dst, needed_bytes);
            this->buf_len += bytes_read;

            if (this->buf_len < 4) {
                return ParseResult::NeedMoreData;
            }

            packet->device_id = DeviceId(this->buf[0]);
            packet->instruction = Instruction(this->buf[3]);

            // byte stuffing can be ignored for the subtractions: the checksum is explicitly not
            // part of the stuffing range and the allowed values for the instruction guarantee that
            // no stuffing can happen for it or the (possibly) following error

            // clang-format off
            this->raw_remaining_data_len = uint16_from_le(buf + 1) 
                - 1                                                     // instruction field
                - (packet->instruction == Instruction::Status)           // error field (only present on status packets)
                - 2;                                                    // crc checksum
            // clang-format on

            this->buf_len = 0;
            this->current_state = ParserState::ErrorField;
        }
        // fallthrough
        case ParserState::ErrorField: {
            if (packet->instruction == Instruction::Status) {
                uint8_t error;
                if (this->receiver.read(cursor, &error, 1) == 0) {
                    return ParseResult::NeedMoreData;
                }

                packet->error = Error(error);
            }

            packet->data.clear();
            this->current_state = ParserState::Data;
        }
        // fallthrough
        case ParserState::Data: {
            auto prev_size = packet->data.size();

            // since there may be stuffing, this length is actually only an upper bound on the
            // number of bytes; it should be close enough though
            if (prev_size + this->raw_remaining_data_len > MAX_PACKET_DATA_LEN) {
                this->current_state = ParserState::Header;
                return ParseResult::BufferOverflow;
            }

            packet->data.resize(prev_size + this->raw_remaining_data_len);
            uint8_t* dst = packet->data.data() + prev_size;

            size_t bytes_read;
            auto raw_bytes_read = this->receiver.read_raw_num_bytes(
                cursor, dst, &bytes_read, this->raw_remaining_data_len);

            packet->data.resize(prev_size + bytes_read);
            this->raw_remaining_data_len -= raw_bytes_read;

            if (this->raw_remaining_data_len > 0) {
                return ParseResult::NeedMoreData;
            }

            this->current_state = ParserState::Checksum;
        }
        // fallthrough
        case ParserState::Checksum: {
            auto needed_bytes = 2 - this->buf_len;

            uint8_t* dst = this->buf + buf_len;
            auto bytes_read = this->receiver.read_raw(cursor, dst, needed_bytes);
            this->buf_len += bytes_read;

            if (this->buf_len < 2) {
                return ParseResult::NeedMoreData;
            }

            auto checksum = uint16_from_le(this->buf);
            this->buf_len = 0;
            this->current_state = ParserState::Header;

            if (checksum == this->receiver.current_crc()) {
                return ParseResult::PacketAvailable;
            } else {
                return ParseResult::MismatchedChecksum;
            }
        }
        default: { return ParseResult::UnknownState; }
    }
}

InstructionPacket::~InstructionPacket() {
    switch (this->instruction) {
        case Instruction::Ping: {
            this->ping.~PingArgs();
            break;
        }
        case Instruction::Read: {
            this->read.~ReadArgs();
            break;
        }
        case Instruction::Write: {
            this->write.~WriteArgs();
            break;
        }
        case Instruction::RegWrite: {
            this->reg_write.~RegWriteArgs();
            break;
        }
        case Instruction::Action: {
            this->action.~ActionArgs();
            break;
        }
        case Instruction::FactoryReset: {
            this->factory_reset.~FactoryResetArgs();
            break;
        }
        case Instruction::Reboot: {
            this->reboot.~RebootArgs();
            break;
        }
        case Instruction::Clear: {
            this->clear.~ClearArgs();
            break;
        }
        case Instruction::Status: {
            break;
        }
        case Instruction::SyncRead: {
            this->sync_read.~SyncReadArgs();
            break;
        }
        case Instruction::SyncWrite: {
            this->sync_write.~SyncWriteArgs();
            break;
        }
        case Instruction::BulkRead: {
            this->bulk_read.~BulkReadArgs();
            break;
        }
        case Instruction::BulkWrite: {
            this->bulk_write.~BulkWriteArgs();
            break;
        }
    }
}

InstructionPacket& InstructionPacket::operator=(InstructionPacket&& rhs) noexcept {
    this->~InstructionPacket();
    this->instruction = rhs.instruction;

    switch (rhs.instruction) {
        case Instruction::Ping: {
            new (&this->ping) PingArgs(std::move(rhs.ping));
            break;
        }
        case Instruction::Read: {
            new (&this->read) ReadArgs(std::move(rhs.read));
            break;
        }
        case Instruction::Write: {
            new (&this->write) WriteArgs(std::move(rhs.write));
            break;
        }
        case Instruction::RegWrite: {
            new (&this->reg_write) RegWriteArgs(std::move(rhs.reg_write));
            break;
        }
        case Instruction::Action: {
            new (&this->action) ActionArgs(std::move(rhs.action));
            break;
        }
        case Instruction::FactoryReset: {
            new (&this->factory_reset) FactoryResetArgs(std::move(rhs.factory_reset));
            break;
        }
        case Instruction::Reboot: {
            new (&this->reboot) RebootArgs(std::move(rhs.reboot));
            break;
        }
        case Instruction::Clear: {
            new (&this->clear) ClearArgs(std::move(rhs.clear));
            break;
        }
        case Instruction::Status: {
            break;
        }
        case Instruction::SyncRead: {
            new (&this->sync_read) SyncReadArgs(std::move(rhs.sync_read));
            break;
        }
        case Instruction::SyncWrite: {
            new (&this->sync_write) SyncWriteArgs(std::move(rhs.sync_write));
            break;
        }
        case Instruction::BulkRead: {
            new (&this->bulk_read) BulkReadArgs(std::move(rhs.bulk_read));
            break;
        }
        case Instruction::BulkWrite: {
            new (&this->bulk_write) BulkWriteArgs(std::move(rhs.bulk_write));
            break;
        }
    }

    return *this;
}

InstructionParseResult
    parse_instruction_packet(const Packet& packet, InstructionPacket* instruction_packet) {
    switch (packet.instruction) {
        case Instruction::Ping: {
            if (packet.data.size() != 0) {
                return InstructionParseResult::InvalidPacketLen;
            }

            *instruction_packet = InstructionPacket(PingArgs{packet.device_id});
            break;
        }
        case Instruction::Read: {
            if (packet.data.size() != 4) {
                return InstructionParseResult::InvalidPacketLen;
            }

            *instruction_packet = InstructionPacket(ReadArgs{
                packet.device_id,
                uint16_from_le(packet.data.data()),
                uint16_from_le(packet.data.data() + 2),
            });

            break;
        }
        case Instruction::Write: {
            if (packet.data.size() < 2) {
                return InstructionParseResult::InvalidPacketLen;
            }

            auto start_addr = uint16_from_le(packet.data.data());

            std::vector<uint8_t> dst_data;
            dst_data.reserve(packet.data.size() - 2);

            std::copy(
                packet.data.data() + 2,
                packet.data.data() + packet.data.size(),
                std::back_inserter(dst_data));

            *instruction_packet = InstructionPacket(WriteArgs{
                packet.device_id,
                start_addr,
                std::move(dst_data),
            });

            break;
        }
        case Instruction::RegWrite: {
            if (packet.data.size() < 2) {
                return InstructionParseResult::InvalidPacketLen;
            }

            auto start_addr = uint16_from_le(packet.data.data());

            std::vector<uint8_t> dst_data;
            dst_data.reserve(packet.data.size() - 2);

            std::copy(
                packet.data.data() + 2,
                packet.data.data() + packet.data.size(),
                std::back_inserter(dst_data));

            *instruction_packet = InstructionPacket(RegWriteArgs{
                packet.device_id,
                start_addr,
                std::move(dst_data),
            });

            break;
        }
        case Instruction::Action: {
            if (packet.data.size() != 0) {
                return InstructionParseResult::InvalidPacketLen;
            }

            *instruction_packet = InstructionPacket(ActionArgs{});
            break;
        }
        case Instruction::FactoryReset: {
            if (packet.data.size() != 1) {
                return InstructionParseResult::InvalidPacketLen;
            }

            *instruction_packet = InstructionPacket(FactoryResetArgs{
                packet.device_id,
                FactoryReset(packet.data[0]),
            });

            break;
        }
        case Instruction::Reboot: {
            if (packet.data.size() != 0) {
                return InstructionParseResult::InvalidPacketLen;
            }

            *instruction_packet = InstructionPacket(RebootArgs{});
            break;
        }
        case Instruction::Clear: {
            if (packet.data.size() != 5) {
                return InstructionParseResult::InvalidPacketLen;
            }

            *instruction_packet = InstructionPacket(ClearArgs{});
            break;
        }
        case Instruction::SyncRead: {
            if (packet.data.size() < 4) {
                return InstructionParseResult::InvalidPacketLen;
            }

            if (!packet.device_id.is_broadcast()) {
                return InstructionParseResult::InvalidDeviceId;
            }

            auto start_addr = uint16_from_le(packet.data.data());
            auto len = uint16_from_le(packet.data.data() + 2);

            std::vector<DeviceId> devices;
            devices.reserve(packet.data.size() - 4);

            for (size_t i = 4; i < packet.data.size(); i++) {
                DeviceId device_id(packet.data[i]);

                if (device_id.is_broadcast()) {
                    return InstructionParseResult::InvalidDeviceId;
                }

                devices.push_back(device_id);
            }

            *instruction_packet = InstructionPacket(SyncReadArgs{
                std::move(devices),
                start_addr,
                len,
            });

            break;
        }
        case Instruction::SyncWrite: {
            if (packet.data.size() < 4) {
                return InstructionParseResult::InvalidPacketLen;
            }

            if (!packet.device_id.is_broadcast()) {
                return InstructionParseResult::InvalidDeviceId;
            }

            auto start_addr = uint16_from_le(packet.data.data());
            auto len = uint16_from_le(packet.data.data() + 2);

            // make sure every entry has full size (id + data)
            if ((packet.data.size() - 4) % (len + 1) != 0) {
                return InstructionParseResult::InvalidPacketLen;
            }

            std::vector<DeviceId> devices;
            std::vector<uint8_t> dst_data;

            auto num_devices = (packet.data.size() - 4) / (len + 1);
            devices.reserve(num_devices);
            dst_data.reserve(num_devices * len);

            for (size_t i = 4; i < packet.data.size(); i += len + 1) {
                DeviceId device_id(packet.data[i]);

                if (device_id.is_broadcast()) {
                    return InstructionParseResult::InvalidDeviceId;
                }

                devices.push_back(device_id);

                std::copy(
                    packet.data.data() + i + 1,
                    packet.data.data() + i + len + 1,
                    std::back_inserter(dst_data));
            }

            *instruction_packet = InstructionPacket(SyncWriteArgs{
                std::move(devices),
                start_addr,
                len,
                std::move(dst_data),
            });

            break;
        }
        case Instruction::BulkRead: {
            if (packet.data.size() % 5 != 0) {
                return InstructionParseResult::InvalidPacketLen;
            }

            if (!packet.device_id.is_broadcast()) {
                return InstructionParseResult::InvalidDeviceId;
            }

            std::vector<ReadArgs> reads;
            reads.reserve(packet.data.size() / 5);

            for (size_t i = 0; i < packet.data.size(); i += 5) {
                DeviceId device_id(packet.data[i]);

                if (device_id.is_broadcast()) {
                    return InstructionParseResult::InvalidDeviceId;
                }

                auto start_addr = uint16_from_le(packet.data.data() + i + 1);
                auto len = uint16_from_le(packet.data.data() + i + 3);

                reads.push_back(ReadArgs{
                    device_id,
                    start_addr,
                    len,
                });
            }

            *instruction_packet = InstructionPacket(BulkReadArgs{std::move(reads)});
            break;
        }
        case Instruction::BulkWrite: {
            if (!packet.device_id.is_broadcast()) {
                return InstructionParseResult::InvalidDeviceId;
            }

            // reservation is an upper bound since every write has 5 bytes of overhead
            std::vector<WriteArgs> writes;
            writes.reserve(packet.data.size() / 5);

            size_t i = 0;
            while (i + 5 <= packet.data.size()) {
                DeviceId device_id(packet.data[i]);

                if (device_id.is_broadcast()) {
                    return InstructionParseResult::InvalidDeviceId;
                }

                auto start_addr = uint16_from_le(packet.data.data() + i + 1);
                auto len = uint16_from_le(packet.data.data() + i + 3);

                if (i + 5 + len > packet.data.size()) {
                    return InstructionParseResult::InvalidPacketLen;
                }

                std::vector<uint8_t> data;
                data.reserve(len);

                std::copy(
                    packet.data.data() + i + 5,
                    packet.data.data() + i + 5 + len,
                    std::back_inserter(data));

                writes.push_back(WriteArgs{
                    device_id,
                    start_addr,
                    data,
                });

                i += 5 + len;
            }

            if (i < packet.data.size()) {
                return InstructionParseResult::InvalidPacketLen;
            }

            *instruction_packet = InstructionPacket(BulkWriteArgs{std::move(writes)});
            break;
        }
        case Instruction::Status: {
            return InstructionParseResult::InstructionIsStatus;
        }
        default: { return InstructionParseResult::UnknownInstruction; }
    }

    return InstructionParseResult::Ok;
}

std::string to_string(const ParseResult& result) {
    switch (result) {
        case ParseResult::PacketAvailable: {
            return "packet available";
        }
        case ParseResult::NeedMoreData: {
            return "need more data";
        }
        case ParseResult::BufferOverflow: {
            return "buffer overflow";
        }
        case ParseResult::MismatchedChecksum: {
            return "mismatched checksum";
        }
        case ParseResult::UnknownState: {
            return "unknown parser state";
        }
        default: { return "unknown parse error"; }
    }
}
