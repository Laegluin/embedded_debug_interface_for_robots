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

    switch (this->last_instruction_packet.instruction) {
        case Instruction::Ping:
        case Instruction::Read: {
            break;
        }
        // writes have status packet responses disabled, so we handle them here
        case Instruction::Write: {
            auto write = [&](DeviceId device_id) -> bool {
                auto& control_table = this->get_control_table(device_id);

                return control_table->write(
                    this->last_instruction_packet.write.start_addr,
                    this->last_instruction_packet.write.data.data(),
                    this->last_instruction_packet.write.data.size());
            };

            auto result = ProtocolResult::Ok;

            if (!this->last_instruction_packet.write.device_id.is_broadcast()) {
                auto is_write_ok = write(this->last_instruction_packet.write.device_id);
                if (!is_write_ok) {
                    result = ProtocolResult::InvalidWrite;
                }
            } else {
                // since this is a broadcast, we write to all devices we know of
                for (auto& pair : this->control_tables) {
                    auto device_id = pair.first;

                    auto is_write_ok = write(device_id);
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
        case Instruction::Clear:
        case Instruction::SyncRead: {
            break;
        }
        // sync writes have status packet responses disabled, so we handle them here
        case Instruction::SyncWrite: {
            auto result = ProtocolResult::Ok;

            for (auto iter = this->last_instruction_packet.sync_write.devices.begin();
                 iter != this->last_instruction_packet.sync_write.devices.end();
                 ++iter) {
                auto device_id = *iter;
                auto& control_table = this->get_control_table(device_id);

                auto idx =
                    std::distance(this->last_instruction_packet.sync_write.devices.begin(), iter);
                auto data_start = this->last_instruction_packet.sync_write.data.data()
                    + (idx * this->last_instruction_packet.sync_write.len);

                auto is_write_ok = control_table->write(
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
            break;
        }
        // bulk writes have status packet responses disabled, so we handle them here
        case Instruction::BulkWrite: {
            auto result = ProtocolResult::Ok;

            for (auto& write_args : this->last_instruction_packet.bulk_write.writes) {
                auto& control_table = this->get_control_table(write_args.device_id);

                auto is_write_ok = control_table->write(
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
            // see `receive_instruction_packet`
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

            auto& control_table = this->get_control_table(status_packet.device_id);

            auto is_write_ok = control_table->write(
                read_args.start_addr, status_packet.data.data(), read_args.len);

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
