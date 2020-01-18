#include "control_table.h"
#include "mx106_control_table.h"
#include "mx64_control_table.h"
#include <algorithm>

static std::unique_ptr<ControlTable>&
    get_control_table(ControlTableMap& control_tables, DeviceId device_id) {
    auto iter = control_tables.find(device_id);

    if (iter != control_tables.end()) {
        return iter->second;
    } else {
        return control_tables.emplace(device_id, std::make_unique<UnknownControlTable>())
            .first->second;
    }
}

static bool is_valid_device_id(DeviceId instruction_device_id, DeviceId status_device_id) {
    if (status_device_id.is_broadcast()) {
        return false;
    }

    return instruction_device_id == status_device_id || instruction_device_id.is_broadcast();
}

static void emplace_control_table(
    DeviceId device_id,
    uint32_t model_number,
    ControlTableMap& control_tables) {
    auto iter = control_tables.find(device_id);

    switch (model_number) {
        case Mx64ControlTable::MODEL_NUMBER: {
            if (iter == control_tables.end()) {
                control_tables.emplace(device_id, std::make_unique<Mx64ControlTable>());
            } else if (iter->second->model_number() != Mx64ControlTable::MODEL_NUMBER) {
                iter->second = std::make_unique<Mx64ControlTable>();
            }
            break;
        }
        case Mx106ControlTable::MODEL_NUMBER: {
            if (iter == control_tables.end()) {
                control_tables.emplace(device_id, std::make_unique<Mx106ControlTable>());
            } else if (iter->second->model_number() != Mx106ControlTable::MODEL_NUMBER) {
                iter->second = std::make_unique<Mx106ControlTable>();
            }
            break;
        }
        default: {
            if (iter == control_tables.end()) {
                control_tables.emplace(device_id, std::make_unique<UnknownControlTable>());
            } else if (iter->second->model_number() != UnknownControlTable::MODEL_NUMBER) {
                iter->second = std::make_unique<UnknownControlTable>();
            }
            break;
        }
    }
}

// TODO: handle disconnected devices
CommResult update_control_table_map(
    const InstructionPacket& instruction_packet,
    const Packet& status_packet,
    ControlTableMap& control_tables) {
    if (status_packet.instruction != Instruction::Status) {
        return CommResult::StatusIsInstruction;
    }

    if (!status_packet.error.is_ok()) {
        return CommResult::StatusHasError;
    }

    switch (instruction_packet.instruction) {
        case Instruction::Ping: {
            if (!is_valid_device_id(instruction_packet.ping.device_id, status_packet.device_id)) {
                return CommResult::InvalidDeviceId;
            }

            if (status_packet.data.size() != 3) {
                return CommResult::InvalidPacketLen;
            }

            auto model_number = uint16_from_le(status_packet.data.data());
            emplace_control_table(status_packet.device_id, model_number, control_tables);
            break;
        }
        case Instruction::Read: {
            if (!is_valid_device_id(instruction_packet.read.device_id, status_packet.device_id)) {
                return CommResult::InvalidDeviceId;
            }

            if (status_packet.data.size() != instruction_packet.read.len) {
                return CommResult::InvalidPacketLen;
            }

            auto& control_table = get_control_table(control_tables, status_packet.device_id);

            auto is_write_ok = control_table->write(
                instruction_packet.read.start_addr,
                status_packet.data.data(),
                instruction_packet.read.len);

            if (!is_write_ok) {
                return CommResult::InvalidWrite;
            }

            break;
        }
        case Instruction::Write: {
            if (!is_valid_device_id(instruction_packet.write.device_id, status_packet.device_id)) {
                return CommResult::InvalidDeviceId;
            }

            if (status_packet.data.size() != 0) {
                return CommResult::InvalidPacketLen;
            }

            auto& control_table = get_control_table(control_tables, status_packet.device_id);

            auto is_write_ok = control_table->write(
                instruction_packet.write.start_addr,
                instruction_packet.write.data.data(),
                instruction_packet.write.data.size());

            if (!is_write_ok) {
                return CommResult::InvalidWrite;
            }

            break;
        }
        case Instruction::RegWrite:
        case Instruction::Action: {
            // TODO: impl RegWrite/Action?
            break;
        }
        case Instruction::FactoryReset: {
            // FIXME: impl
            break;
        }
        case Instruction::Reboot: {
            // TODO: simply issue a reset here
            break;
        }
        case Instruction::Clear: {
            // TODO: impl as ControlTable method
            break;
        }
        case Instruction::SyncRead: {
            auto iter = std::find(
                instruction_packet.sync_read.devices.begin(),
                instruction_packet.sync_read.devices.end(),
                status_packet.device_id);

            if (iter == instruction_packet.sync_read.devices.end()) {
                return CommResult::InvalidDeviceId;
            }

            if (status_packet.data.size() != instruction_packet.sync_read.len) {
                return CommResult::InvalidPacketLen;
            }

            auto& control_table = get_control_table(control_tables, status_packet.device_id);

            auto is_write_ok = control_table->write(
                instruction_packet.sync_read.start_addr,
                status_packet.data.data(),
                instruction_packet.sync_read.len);

            if (!is_write_ok) {
                return CommResult::InvalidWrite;
            }

            break;
        }
        case Instruction::SyncWrite: {
            auto iter = std::find(
                instruction_packet.sync_write.devices.begin(),
                instruction_packet.sync_write.devices.end(),
                status_packet.device_id);

            if (iter == instruction_packet.sync_write.devices.end()) {
                return CommResult::InvalidDeviceId;
            }

            if (status_packet.data.size() != 0) {
                return CommResult::InvalidPacketLen;
            }

            auto& control_table = get_control_table(control_tables, status_packet.device_id);

            auto idx = std::distance(instruction_packet.sync_write.devices.begin(), iter);
            auto data_start = instruction_packet.sync_write.data.data()
                + (idx * instruction_packet.sync_write.len);

            auto is_write_ok = control_table->write(
                instruction_packet.sync_write.start_addr,
                data_start,
                instruction_packet.sync_write.len);

            if (!is_write_ok) {
                return CommResult::InvalidWrite;
            }

            break;
        }
        case Instruction::BulkRead: {
            auto iter = std::find_if(
                instruction_packet.bulk_read.reads.begin(),
                instruction_packet.bulk_read.reads.end(),
                [&](const ReadArgs& read_args) {
                    return read_args.device_id == status_packet.device_id;
                });

            if (iter == instruction_packet.bulk_read.reads.end()) {
                return CommResult::InvalidDeviceId;
            }

            auto& read_args = *iter;

            if (status_packet.data.size() != read_args.len) {
                return CommResult::InvalidPacketLen;
            }

            auto& control_table = get_control_table(control_tables, status_packet.device_id);

            auto is_write_ok = control_table->write(
                read_args.start_addr, status_packet.data.data(), read_args.len);

            if (!is_write_ok) {
                return CommResult::InvalidWrite;
            }

            break;
        }
        case Instruction::BulkWrite: {
            auto iter = std::find_if(
                instruction_packet.bulk_write.writes.begin(),
                instruction_packet.bulk_write.writes.end(),
                [&](const WriteArgs& read_args) {
                    return read_args.device_id == status_packet.device_id;
                });

            if (iter == instruction_packet.bulk_write.writes.end()) {
                return CommResult::InvalidDeviceId;
            }

            auto& write_args = *iter;

            if (status_packet.data.size() != 0) {
                return CommResult::InvalidPacketLen;
            }

            auto& control_table = get_control_table(control_tables, status_packet.device_id);

            auto is_write_ok = control_table->write(
                write_args.start_addr, write_args.data.data(), write_args.data.size());

            if (!is_write_ok) {
                return CommResult::InvalidWrite;
            }

            break;
        }
        case Instruction::Status: {
            return CommResult::InstructionIsStatus;
        }
        default: { return CommResult::UnknownInstruction; }
    }

    return CommResult::Ok;
}
