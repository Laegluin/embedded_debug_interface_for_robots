#include "control_table.h"
#include "device/mx106.h"
#include "device/mx64.h"
#include <algorithm>

ControlTableMap::ControlTableMap() : is_last_instruction_packet_known(false) {
    // TODO: remove after UI is done
    this->control_tables.emplace(DeviceId(0), std::make_unique<Mx64ControlTable>());
    this->control_tables.emplace(DeviceId(1), std::make_unique<Mx106ControlTable>());
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

    return ProtocolResult::Ok;
}

// TODO: handle disconnected devices
ProtocolResult ControlTableMap::receive_status_packet(const Packet& status_packet) {
    if (status_packet.instruction != Instruction::Status) {
        return ProtocolResult::StatusIsInstruction;
    }

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
            this->register_control_table(status_packet.device_id, model_number);
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

            auto& control_table = this->get_control_table(status_packet.device_id);

            auto is_write_ok = control_table->write(
                this->last_instruction_packet.read.start_addr,
                status_packet.data.data(),
                this->last_instruction_packet.read.len);

            if (!is_write_ok) {
                return ProtocolResult::InvalidWrite;
            }

            break;
        }
        case Instruction::Write: {
            if (status_packet.device_id.is_broadcast()
                || (this->last_instruction_packet.write.device_id != status_packet.device_id
                    && !this->last_instruction_packet.write.device_id.is_broadcast())) {
                return ProtocolResult::InvalidDeviceId;
            }

            if (status_packet.data.size() != 0) {
                return ProtocolResult::InvalidPacketLen;
            }

            auto& control_table = this->get_control_table(status_packet.device_id);

            auto is_write_ok = control_table->write(
                this->last_instruction_packet.write.start_addr,
                this->last_instruction_packet.write.data.data(),
                this->last_instruction_packet.write.data.size());

            if (!is_write_ok) {
                return ProtocolResult::InvalidWrite;
            }

            break;
        }
        case Instruction::RegWrite:
        case Instruction::Action:
        case Instruction::FactoryReset:
        case Instruction::Reboot:
        case Instruction::Clear: {
            // We are only really interested in reads and writes, since those make up most of the
            // traffic. So we simply ignore all other packet types.
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

            auto& control_table = this->get_control_table(status_packet.device_id);

            auto is_write_ok = control_table->write(
                this->last_instruction_packet.sync_read.start_addr,
                status_packet.data.data(),
                this->last_instruction_packet.sync_read.len);

            if (!is_write_ok) {
                return ProtocolResult::InvalidWrite;
            }

            break;
        }
        case Instruction::SyncWrite: {
            auto iter = std::find(
                this->last_instruction_packet.sync_write.devices.begin(),
                this->last_instruction_packet.sync_write.devices.end(),
                status_packet.device_id);

            if (iter == this->last_instruction_packet.sync_write.devices.end()) {
                return ProtocolResult::InvalidDeviceId;
            }

            if (status_packet.data.size() != 0) {
                return ProtocolResult::InvalidPacketLen;
            }

            auto& control_table = this->get_control_table(status_packet.device_id);

            auto idx =
                std::distance(this->last_instruction_packet.sync_write.devices.begin(), iter);
            auto data_start = this->last_instruction_packet.sync_write.data.data()
                + (idx * this->last_instruction_packet.sync_write.len);

            auto is_write_ok = control_table->write(
                this->last_instruction_packet.sync_write.start_addr,
                data_start,
                this->last_instruction_packet.sync_write.len);

            if (!is_write_ok) {
                return ProtocolResult::InvalidWrite;
            }

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

            auto& control_table = this->get_control_table(status_packet.device_id);

            auto is_write_ok = control_table->write(
                read_args.start_addr, status_packet.data.data(), read_args.len);

            if (!is_write_ok) {
                return ProtocolResult::InvalidWrite;
            }

            break;
        }
        case Instruction::BulkWrite: {
            auto iter = std::find_if(
                this->last_instruction_packet.bulk_write.writes.begin(),
                this->last_instruction_packet.bulk_write.writes.end(),
                [&](const WriteArgs& read_args) {
                    return read_args.device_id == status_packet.device_id;
                });

            if (iter == this->last_instruction_packet.bulk_write.writes.end()) {
                return ProtocolResult::InvalidDeviceId;
            }

            auto& write_args = *iter;

            if (status_packet.data.size() != 0) {
                return ProtocolResult::InvalidPacketLen;
            }

            auto& control_table = this->get_control_table(status_packet.device_id);

            auto is_write_ok = control_table->write(
                write_args.start_addr, write_args.data.data(), write_args.data.size());

            if (!is_write_ok) {
                return ProtocolResult::InvalidWrite;
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

void ControlTableMap::register_control_table(DeviceId device_id, uint32_t model_number) {
    auto iter = this->control_tables.find(device_id);

    switch (model_number) {
        case Mx64ControlTable::MODEL_NUMBER: {
            if (iter == this->control_tables.end()) {
                this->control_tables.emplace(device_id, std::make_unique<Mx64ControlTable>());
            } else if (iter->second->model_number() != Mx64ControlTable::MODEL_NUMBER) {
                iter->second = std::make_unique<Mx64ControlTable>();
            }
            break;
        }
        case Mx106ControlTable::MODEL_NUMBER: {
            if (iter == this->control_tables.end()) {
                this->control_tables.emplace(device_id, std::make_unique<Mx106ControlTable>());
            } else if (iter->second->model_number() != Mx106ControlTable::MODEL_NUMBER) {
                iter->second = std::make_unique<Mx106ControlTable>();
            }
            break;
        }
        default: {
            if (iter == this->control_tables.end()) {
                this->control_tables.emplace(device_id, std::make_unique<UnknownControlTable>());
            } else if (iter->second->model_number() != UnknownControlTable::MODEL_NUMBER) {
                iter->second = std::make_unique<UnknownControlTable>();
            }
            break;
        }
    }
}

std::unique_ptr<ControlTable>& ControlTableMap::get_control_table(DeviceId device_id) {
    auto iter = this->control_tables.find(device_id);

    if (iter != this->control_tables.end()) {
        return iter->second;
    } else {
        return this->control_tables.emplace(device_id, std::make_unique<UnknownControlTable>())
            .first->second;
    }
}
