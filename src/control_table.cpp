#include "control_table.h"
#include "device/mx106.h"
#include "device/mx64.h"
#include <algorithm>

Segment Segment::new_data(uint16_t start_addr, uint16_t len) {
    Segment segment;
    segment.type = Type::DataSegment;
    segment.data.start_addr = start_addr;
    segment.data.len = len;
    segment.data.data = nullptr;

    return segment;
}

Segment
    Segment::new_indirect_address(uint16_t data_start_addr, uint16_t map_start_addr, uint16_t len) {
    Segment segment;
    segment.type = Type::IndirectAddressSegment;
    segment.indirect_address.data_start_addr = data_start_addr;
    segment.indirect_address.map_start_addr = map_start_addr;
    segment.indirect_address.len = len;
    segment.indirect_address.map = nullptr;

    return segment;
}

uint16_t Segment::start_addr() const {
    switch (this->type) {
        case Type::DataSegment: {
            return this->data.start_addr;
        }
        case Type::IndirectAddressSegment: {
            return this->indirect_address.map_start_addr;
        }
        default: { return 0; }
    }
}

uint16_t Segment::len() const {
    switch (this->type) {
        case Type::DataSegment: {
            return this->data.len;
        }
        case Type::IndirectAddressSegment: {
            return this->indirect_address.len;
        }
        default: { return 0; }
    }
}

bool Segment::resolve_addr(uint16_t addr, uint16_t* resolved_addr) const {
    if (this->type != Type::IndirectAddressSegment) {
        return false;
    }

    // not part of mapped address space
    if (addr < this->indirect_address.data_start_addr
        || addr >= this->indirect_address.data_start_addr + this->indirect_address.len / 2) {
        return false;
    }

    auto addr_idx = addr - this->indirect_address.data_start_addr;
    auto map_idx = addr_idx * 2;
    *resolved_addr = uint16_from_le(this->indirect_address.map + map_idx);
    return true;
}

void Segment::set_backing_storage(uint8_t* buf) {
    switch (this->type) {
        case Type::DataSegment: {
            this->data.data = buf;
            break;
        }
        case Type::IndirectAddressSegment: {
            this->indirect_address.map = buf;
            break;
        }
    }
}

bool Segment::read(uint16_t addr, uint8_t* byte) const {
    if (addr < this->start_addr() || addr >= this->start_addr() + this->len()) {
        return false;
    }

    switch (this->type) {
        case Type::DataSegment: {
            *byte = this->data.data[addr - this->data.start_addr];
            return true;
        }
        case Type::IndirectAddressSegment: {
            *byte = this->indirect_address.map[addr - this->indirect_address.map_start_addr];
            return true;
        }
        default: { return false; }
    }
}

bool Segment::write(uint16_t addr, uint8_t byte) {
    if (addr < this->start_addr() || addr >= this->start_addr() + this->len()) {
        return false;
    }

    switch (this->type) {
        case Type::DataSegment: {
            this->data.data[addr - this->data.start_addr] = byte;
            return true;
        }
        case Type::IndirectAddressSegment: {
            this->indirect_address.map[addr - this->indirect_address.map_start_addr] = byte;
            return true;
        }
        default: { return false; }
    }
}

ControlTableMemory::ControlTableMemory(std::vector<Segment>&& segments) : segments(segments) {
    std::sort(this->segments.begin(), this->segments.end(), [](auto& lhs, auto& rhs) {
        return lhs.start_addr() < rhs.start_addr();
    });

    size_t buf_len = std::accumulate(
        this->segments.begin(), this->segments.end(), 0, [](size_t acc, auto& segment) {
            return acc + segment.len();
        });

    this->buf.resize(buf_len);

    size_t offset = 0;

    for (auto& segment : this->segments) {
        segment.set_backing_storage(this->buf.data() + offset);
        offset += segment.len();
    }
}

ControlTableMemory::ControlTableMemory(const ControlTableMemory& src) :
    segments(src.segments),
    buf(src.buf) {
    // need to change the backing storage pointers to point to our copy
    size_t offset = 0;

    for (auto& segment : this->segments) {
        segment.set_backing_storage(this->buf.data() + offset);
        offset += segment.len();
    }
}

bool ControlTableMemory::read_uint8(uint16_t addr, uint8_t* dst) const {
    return this->read(addr, dst, 1);
}

bool ControlTableMemory::read_uint16(uint16_t addr, uint16_t* dst) const {
    uint8_t buf[2];

    if (this->read(addr, buf, 2)) {
        *dst = uint16_from_le(buf);
        return true;
    } else {
        return false;
    }
}

bool ControlTableMemory::read_uint32(uint16_t addr, uint32_t* dst) const {
    uint8_t buf[4];

    if (this->read(addr, buf, 4)) {
        *dst = uint32_from_le(buf);
        return true;
    } else {
        return false;
    }
}

bool ControlTableMemory::read(uint16_t start_addr, uint8_t* dst, uint16_t len) const {
    for (uint16_t addr = start_addr; addr < start_addr + len; addr++) {
        uint16_t resolved_addr = this->resolve_addr(addr);
        bool is_read_ok = false;

        for (auto& segment : this->segments) {
            if (segment.read(resolved_addr, dst)) {
                is_read_ok = true;
                break;
            }
        }

        if (!is_read_ok) {
            return false;
        }

        dst++;
    }

    return true;
}

bool ControlTableMemory::write_uint8(uint16_t addr, uint8_t value) {
    return this->write(addr, &value, 1);
}

bool ControlTableMemory::write_uint16(uint16_t addr, uint16_t value) {
    uint8_t bytes[2];
    uint16_to_le(bytes, value);
    return this->write(addr, bytes, 2);
}

bool ControlTableMemory::write_uint32(uint16_t addr, uint32_t value) {
    uint8_t bytes[4];
    uint32_to_le(bytes, value);
    return this->write(addr, bytes, 4);
}

bool ControlTableMemory::write(uint16_t start_addr, const uint8_t* buf, uint16_t len) {
    for (uint16_t addr = start_addr; addr < start_addr + len; addr++) {
        uint16_t resolved_addr = this->resolve_addr(addr);
        bool is_write_ok = false;

        for (auto& segment : this->segments) {
            if (segment.write(resolved_addr, *buf)) {
                is_write_ok = true;
                break;
            }
        }

        if (!is_write_ok) {
            return false;
        }

        buf++;
    }

    return true;
}

uint16_t ControlTableMemory::resolve_addr(uint16_t addr) const {
    // documentation is not really clear on whether addresses can be resolved multiple times;
    // we simply assume it is not since that's faster (and the most likely case anyway)
    for (auto& segment : this->segments) {
        uint16_t resolved_addr;
        if (segment.resolve_addr(addr, &resolved_addr)) {
            return resolved_addr;
        }
    }

    return addr;
}

bool ControlTable::is_unknown_model() const {
    return false;
}

bool ControlTable::write(uint16_t start_addr, const uint8_t* buf, uint16_t len) {
    return this->memory().write(start_addr, buf, len);
}

std::vector<std::pair<const char*, std::string>> ControlTable::fmt_fields() const {
    auto& mem = this->memory();
    auto& fields = this->fields();

    std::vector<std::pair<const char*, std::string>> formatted_fields;
    formatted_fields.reserve(fields.size());

    for (auto& field : fields) {
        switch (field.type) {
            case ControlTableField::FieldType::UInt8: {
                if (!field.uint8.fmt) {
                    continue;
                }

                uint8_t value;
                if (mem.read_uint8(field.addr, &value)) {
                    auto formatted_value = field.uint8.fmt(value);
                    formatted_fields.emplace_back(field.name, std::move(formatted_value));
                }

                break;
            }
            case ControlTableField::FieldType::UInt16: {
                if (!field.uint16.fmt) {
                    continue;
                }

                uint16_t value;
                if (mem.read_uint16(field.addr, &value)) {
                    auto formatted_value = field.uint16.fmt(value);
                    formatted_fields.emplace_back(field.name, std::move(formatted_value));
                }

                break;
            }
            case ControlTableField::FieldType::UInt32: {
                if (!field.uint32.fmt) {
                    continue;
                }

                uint32_t value;
                if (mem.read_uint32(field.addr, &value)) {
                    auto formatted_value = field.uint32.fmt(value);
                    formatted_fields.emplace_back(field.name, std::move(formatted_value));
                }

                break;
            }
        }
    }

    return formatted_fields;
}

ControlTableMap::ControlTableMap() : is_last_instruction_packet_known(false) {}

bool ControlTableMap::is_disconnected(DeviceId device_id) const {
    auto& entry = this->num_missed_packets.get(device_id);
    if (!entry.is_present()) {
        return false;
    }

    return entry.value() > MAX_ALLOWED_MISSED_PACKETS;
}

ProtocolResult ControlTableMap::receive(const Packet& packet) {
    if (packet.instruction == Instruction::Status) {
        return this->receive_status_packet(packet);

    } else {
        return this->receive_instruction_packet(packet);
    }
}

ProtocolResult ControlTableMap::receive_instruction_packet(const Packet& instruction_packet) {
    auto result = parse_instruction_packet(instruction_packet, &this->last_instruction_packet);
    this->is_last_instruction_packet_known = result == InstructionParseResult::Ok;

    if (result != InstructionParseResult::Ok) {
        return ProtocolResult::InvalidInstructionPacket;
    }

    // check if there are any devices that did not respond to the last instruction
    for (auto device_id : this->pending_responses) {
        this->num_missed_packets.get(device_id).or_insert(0)++;
    }

    this->pending_responses.clear();

    // writes have status packet responses disabled, so we handle them here;
    // in addition, we can set the pending responses for reads
    switch (this->last_instruction_packet.instruction) {
        case Instruction::Ping: {
            // we don't know what devices are supposed to answer (that's the whole point of a ping)
            break;
        }
        case Instruction::Read: {
            if (!this->last_instruction_packet.read.device_id.is_broadcast()) {
                this->pending_responses.push_back(this->last_instruction_packet.read.device_id);
            } else {
                for (auto pair : this->control_tables) {
                    auto device_id = pair.first;
                    this->pending_responses.push_back(device_id);
                }
            }

            break;
        }
        case Instruction::Write: {
            auto result = ProtocolResult::Ok;

            if (!this->last_instruction_packet.write.device_id.is_broadcast()) {
                auto& control_table =
                    this->get_or_insert(this->last_instruction_packet.write.device_id);

                auto is_write_ok = control_table.write(
                    this->last_instruction_packet.write.start_addr,
                    this->last_instruction_packet.write.data.data(),
                    this->last_instruction_packet.write.data.size());

                if (!is_write_ok) {
                    result = ProtocolResult::InvalidWrite;
                }
            } else {
                // since this is a broadcast, we write to all devices we know of
                for (auto pair : this->control_tables) {
                    auto device_id = pair.first;
                    auto& control_table = this->get_or_insert(device_id);

                    auto is_write_ok = control_table.write(
                        this->last_instruction_packet.write.start_addr,
                        this->last_instruction_packet.write.data.data(),
                        this->last_instruction_packet.write.data.size());

                    if (!is_write_ok) {
                        result = ProtocolResult::InvalidWrite;
                    }
                }
            }

            if (result != ProtocolResult::Ok) {
                return result;
            }

            break;
        }
        case Instruction::RegWrite:
        case Instruction::Action:
        case Instruction::FactoryReset:
        case Instruction::Reboot:
        case Instruction::Clear: {
            // these instructions are ignored for simplicity
            break;
        }
        case Instruction::SyncRead: {
            this->pending_responses = this->last_instruction_packet.sync_read.devices;
            break;
        }
        case Instruction::SyncWrite: {
            auto result = ProtocolResult::Ok;

            for (auto iter = this->last_instruction_packet.sync_write.devices.begin();
                 iter != this->last_instruction_packet.sync_write.devices.end();
                 ++iter) {
                auto device_id = *iter;
                auto& control_table = this->get_or_insert(device_id);

                auto idx =
                    std::distance(this->last_instruction_packet.sync_write.devices.begin(), iter);
                auto data_start = this->last_instruction_packet.sync_write.data.data()
                    + (idx * this->last_instruction_packet.sync_write.len);

                auto is_write_ok = control_table.write(
                    this->last_instruction_packet.sync_write.start_addr,
                    data_start,
                    this->last_instruction_packet.sync_write.len);

                if (!is_write_ok) {
                    result = ProtocolResult::InvalidWrite;
                }
            }

            if (result != ProtocolResult::Ok) {
                return result;
            }

            break;
        }
        case Instruction::BulkRead: {
            for (auto& read_arg : this->last_instruction_packet.bulk_read.reads) {
                this->pending_responses.push_back(read_arg.device_id);
            }

            break;
        }
        case Instruction::BulkWrite: {
            auto result = ProtocolResult::Ok;

            for (auto& write_args : this->last_instruction_packet.bulk_write.writes) {
                auto& control_table = this->get_or_insert(write_args.device_id);

                auto is_write_ok = control_table.write(
                    write_args.start_addr, write_args.data.data(), write_args.data.size());

                if (!is_write_ok) {
                    result = ProtocolResult::InvalidWrite;
                }
            }

            if (result != ProtocolResult::Ok) {
                return result;
            }

            break;
        }
        case Instruction::Status: {
            return ProtocolResult::InstructionIsStatus;
        }
        default: { return ProtocolResult::UnknownInstruction; }
    }

    return ProtocolResult::Ok;
}

ProtocolResult ControlTableMap::receive_status_packet(const Packet& status_packet) {
    if (status_packet.instruction != Instruction::Status) {
        return ProtocolResult::StatusIsInstruction;
    }

    // Remove the device from the pending responses.
    // This only removes the first match, both for performance and because it is
    // not strictly specified whether instructions like `BulkRead` can address the same
    // device more than once. In this case this would not remove all occurrences and thus
    // make sure that the correct amount of status packets are counted.
    for (size_t i = 0; i < this->pending_responses.size(); i++) {
        if (this->pending_responses[i] == status_packet.device_id) {
            // use swap to avoid copying elements (we don't care about the order since this
            // is just a really small set)
            std::iter_swap(this->pending_responses.begin() + i, this->pending_responses.end() - 1);
            this->pending_responses.pop_back();
            break;
        }
    }

    // All reset this counter for missed packets, since we just got one. Like above,
    // an error in the packet does not matter. We only want to know if the device is
    // actually connected to the bus, not if everything's working correctly.
    this->num_missed_packets.get(status_packet.device_id).or_insert(0) = 0;

    if (!status_packet.error.is_ok()) {
        return ProtocolResult::StatusHasError;
    }

    // nothing we can do if we missed the last instruction
    if (!this->is_last_instruction_packet_known) {
        return ProtocolResult::Ok;
    }

    switch (this->last_instruction_packet.instruction) {
        case Instruction::Ping: {
            if (status_packet.device_id.is_broadcast()
                || (this->last_instruction_packet.ping.device_id != status_packet.device_id
                    && !this->last_instruction_packet.ping.device_id.is_broadcast())) {
                return ProtocolResult::InvalidDeviceId;
            }

            if (status_packet.data.size() != 3) {
                return ProtocolResult::InvalidPacketLen;
            }

            auto model_number = uint16_from_le(status_packet.data.data());
            auto firmware_version = status_packet.data[2];

            auto& control_table =
                this->register_control_table(status_packet.device_id, model_number);
            control_table.set_firmware_version(firmware_version);

            break;
        }
        case Instruction::Read: {
            if (status_packet.device_id.is_broadcast()
                || (this->last_instruction_packet.read.device_id != status_packet.device_id
                    && !this->last_instruction_packet.read.device_id.is_broadcast())) {
                return ProtocolResult::InvalidDeviceId;
            }

            if (status_packet.data.size() != this->last_instruction_packet.read.len) {
                return ProtocolResult::InvalidPacketLen;
            }

            auto& control_table = this->get_or_insert(status_packet.device_id);

            auto is_write_ok = control_table.write(
                this->last_instruction_packet.read.start_addr,
                status_packet.data.data(),
                this->last_instruction_packet.read.len);

            if (!is_write_ok) {
                return ProtocolResult::InvalidWrite;
            }

            break;
        }
        case Instruction::Write: {
            // see `receive_instruction_packet`
            break;
        }
        case Instruction::RegWrite:
        case Instruction::Action:
        case Instruction::FactoryReset:
        case Instruction::Reboot:
        case Instruction::Clear: {
            // We are only really interested in reads and writes, since those make up most of the
            // traffic, so these instructions are ignored for simplicity
            break;
        }
        case Instruction::SyncRead: {
            auto iter = std::find(
                this->last_instruction_packet.sync_read.devices.begin(),
                this->last_instruction_packet.sync_read.devices.end(),
                status_packet.device_id);

            if (iter == this->last_instruction_packet.sync_read.devices.end()) {
                return ProtocolResult::InvalidDeviceId;
            }

            if (status_packet.data.size() != this->last_instruction_packet.sync_read.len) {
                return ProtocolResult::InvalidPacketLen;
            }

            auto& control_table = this->get_or_insert(status_packet.device_id);

            auto is_write_ok = control_table.write(
                this->last_instruction_packet.sync_read.start_addr,
                status_packet.data.data(),
                this->last_instruction_packet.sync_read.len);

            if (!is_write_ok) {
                return ProtocolResult::InvalidWrite;
            }

            break;
        }
        case Instruction::SyncWrite: {
            // see `receive_instruction_packet`
            break;
        }
        case Instruction::BulkRead: {
            auto iter = std::find_if(
                this->last_instruction_packet.bulk_read.reads.begin(),
                this->last_instruction_packet.bulk_read.reads.end(),
                [&](const ReadArgs& read_args) {
                    return read_args.device_id == status_packet.device_id;
                });

            if (iter == this->last_instruction_packet.bulk_read.reads.end()) {
                return ProtocolResult::InvalidDeviceId;
            }

            auto& read_args = *iter;

            if (status_packet.data.size() != read_args.len) {
                return ProtocolResult::InvalidPacketLen;
            }

            auto& control_table = this->get_or_insert(status_packet.device_id);

            auto is_write_ok =
                control_table.write(read_args.start_addr, status_packet.data.data(), read_args.len);

            if (!is_write_ok) {
                return ProtocolResult::InvalidWrite;
            }

            break;
        }
        case Instruction::BulkWrite: {
            // see `receive_instruction_packet`
            break;
        }
        case Instruction::Status: {
            return ProtocolResult::InstructionIsStatus;
        }
        default: { return ProtocolResult::UnknownInstruction; }
    }

    return ProtocolResult::Ok;
}

ControlTable& ControlTableMap::register_control_table(DeviceId device_id, uint16_t model_number) {
    auto& entry = this->control_tables.get(device_id);

    if (!entry.is_present() || entry.value()->is_unknown_model()
        || entry.value()->model_number() != model_number) {
        // for a new `ControlTable` implementation simply add another case
        switch (model_number) {
            case Mx64ControlTable::MODEL_NUMBER: {
                entry.set_value(std::make_unique<Mx64ControlTable>());
                break;
            }
            case Mx106ControlTable::MODEL_NUMBER: {
                entry.set_value(std::make_unique<Mx106ControlTable>());
                break;
            }
            default: {
                entry.set_value(std::make_unique<UnknownControlTable>(model_number));
                break;
            }
        }
    }

    return *entry.value();
}

ControlTable& ControlTableMap::get_or_insert(DeviceId device_id) {
    return *this->control_tables.get(device_id).or_insert_with(
        []() { return std::make_unique<UnknownControlTable>(); });
}

std::string to_string(const ProtocolResult& result) {
    switch (result) {
        case ProtocolResult::Ok: {
            return "ok";
        }
        case ProtocolResult::StatusIsInstruction: {
            return "expected status packet but found instruction packet";
        }
        case ProtocolResult::StatusHasError: {
            return "status packet has an error";
        }
        case ProtocolResult::InstructionIsStatus: {
            return "expected instruction packet but found status packet";
        }
        case ProtocolResult::UnknownInstruction: {
            return "unknown instruction";
        }
        case ProtocolResult::InvalidPacketLen: {
            return "invalid packet length";
        }
        case ProtocolResult::InvalidDeviceId: {
            return "invalid device id";
        }
        case ProtocolResult::InvalidWrite: {
            return "invalid write";
        }
        case ProtocolResult::InvalidInstructionPacket: {
            return "invalid instruction packet";
        }
        default: { return "unknown protocol error"; }
    }
}
