#ifndef CONTROL_TABLE_H
#define CONTROL_TABLE_H

#include "endian_convert.h"
#include "parser.h"
#include <limits>
#include <memory>
#include <stdint.h>
#include <string.h>
#include <string>
#include <unordered_map>

class ControlTable {
  public:
    /// Returns the model number of the device. Any model number greater than
    /// `std::numeric_limits<uint16_t>::max()` does not refer to an actual device.
    virtual uint32_t model_number() const = 0;

    virtual const char* device_name() const = 0;

    virtual bool write(uint16_t start_addr, const uint8_t* bytes, uint16_t len) = 0;

    virtual std::vector<std::pair<const char*, std::string>> fmt_fields() const = 0;
};

class UnknownControlTable : public ControlTable {
  public:
    static const uint32_t MODEL_NUMBER = std::numeric_limits<uint32_t>::max();

    uint32_t model_number() const final {
        return MODEL_NUMBER;
    }

    const char* device_name() const final {
        return "<unknown>";
    }

    bool write(uint16_t, const uint8_t*, uint16_t) final {
        return true;
    }

    std::vector<std::pair<const char*, std::string>> fmt_fields() const final {
        return std::vector<std::pair<const char*, std::string>>();
    }
};

template <uint16_t MAP_START, uint16_t DATA_START, uint16_t LEN>
class AddressMap {
  public:
    AddressMap() {
        memset(this->map, 0, LEN * 2);
    }

    bool write(uint16_t start_addr, const uint8_t* bytes, uint16_t len) {
        if (this->is_valid_map_addr(start_addr) && this->is_valid_map_addr(start_addr + len - 1)) {
            auto idx = start_addr - MAP_START;
            memcpy(this->map + idx, bytes, len);
            return true;
        } else {
            return false;
        }
    }

    bool write_uint16(uint16_t addr, uint16_t val) {
        uint8_t bytes[2];
        uint16_to_le(bytes, val);
        return this->write(addr, bytes, 2);
    }

    uint16_t resolve_addr(uint16_t addr) const {
        if (this->is_valid_data_addr(addr)) {
            auto addr_idx = addr - DATA_START;
            auto map_idx = addr_idx * 2;
            return uint16_from_le(this->map + map_idx);
        } else {
            return addr;
        }
    }

  private:
    bool is_valid_map_addr(uint16_t addr) const {
        return addr >= MAP_START && addr < MAP_START + LEN * 2;
    }

    bool is_valid_data_addr(uint16_t addr) const {
        return addr >= DATA_START && addr < DATA_START + LEN;
    }

    uint8_t map[LEN * 2];
};

template <uint16_t START, uint16_t LEN>
class DataSegment {
  public:
    DataSegment() {
        memset(this->data, 0, LEN);
    }

    bool write(uint16_t start_addr, const uint8_t* bytes, uint16_t len) {
        if (this->is_valid_addr(start_addr) && this->is_valid_addr(start_addr + len - 1)) {
            auto idx = start_addr - START;
            memcpy(this->data + idx, bytes, len);
            return true;
        } else {
            return false;
        }
    }

    bool write_uint8(uint16_t addr, uint8_t val) {
        return this->write(addr, &val, 1);
    }

    bool write_uint16(uint16_t addr, uint16_t val) {
        uint8_t bytes[2];
        uint16_to_le(bytes, val);
        return this->write(addr, bytes, 2);
    }

    bool write_uint32(uint16_t addr, uint32_t val) {
        uint8_t bytes[4];
        uint32_to_le(bytes, val);
        return this->write(addr, bytes, 4);
    }

    uint8_t uint8_at(uint16_t addr) const {
        return this->data[addr - START];
    }

    uint16_t uint16_at(uint16_t addr) const {
        return uint16_from_le(this->data + (addr - START));
    }

    uint32_t uint32_at(uint16_t addr) const {
        return uint32_from_le(this->data + (addr - START));
    }

  private:
    bool is_valid_addr(uint16_t addr) const {
        return addr >= START && addr < START + LEN;
    }

    uint8_t data[LEN];
};

struct ControlTableField {
    enum class FieldType {
        UInt8,
        UInt16,
        UInt32,
    };

    static ControlTableField new_uint8(
        uint16_t addr,
        const char* name,
        uint8_t default_value,
        std::string (*fmt)(uint8_t)) {
        ControlTableField field;
        field.addr = addr;
        field.type = FieldType::UInt8;
        field.name = name;
        field.uint8.default_value = default_value;
        field.uint8.fmt = fmt;
        return field;
    }

    static ControlTableField new_uint16(
        uint16_t addr,
        const char* name,
        uint16_t default_value,
        std::string (*fmt)(uint16_t)) {
        ControlTableField field;
        field.addr = addr;
        field.type = FieldType::UInt16;
        field.name = name;
        field.uint16.default_value = default_value;
        field.uint16.fmt = fmt;
        return field;
    }

    static ControlTableField new_uint32(
        uint16_t addr,
        const char* name,
        uint32_t default_value,
        std::string (*fmt)(uint32_t)) {
        ControlTableField field;
        field.addr = addr;
        field.type = FieldType::UInt32;
        field.name = name;
        field.uint32.default_value = default_value;
        field.uint32.fmt = fmt;
        return field;
    }

    uint16_t addr;
    FieldType type;
    const char* name;
    union {
        struct {
            uint8_t default_value;
            std::string (*fmt)(uint8_t);
        } uint8;
        struct {
            uint16_t default_value;
            std::string (*fmt)(uint16_t);
        } uint16;
        struct {
            uint32_t default_value;
            std::string (*fmt)(uint32_t);
        } uint32;
    };
};

template <typename Data, size_t NUM_FIELDS>
void default_init_control_table(
    Data& data,
    const std::array<ControlTableField, NUM_FIELDS>& fields) {
    for (auto& field : fields) {
        switch (field.type) {
            case ControlTableField::FieldType::UInt8: {
                data.write_uint8(field.addr, field.uint8.default_value);
                break;
            }
            case ControlTableField::FieldType::UInt16: {
                data.write_uint16(field.addr, field.uint16.default_value);
                break;
            }
            case ControlTableField::FieldType::UInt32: {
                data.write_uint32(field.addr, field.uint32.default_value);
                break;
            }
        }
    }
}

template <typename Data, size_t NUM_FIELDS>
std::vector<std::pair<const char*, std::string>> fmt_control_table_fields(
    const Data& data,
    const std::array<ControlTableField, NUM_FIELDS>& fields) {
    std::vector<std::pair<const char*, std::string>> formatted_fields;
    formatted_fields.reserve(NUM_FIELDS);

    for (auto& field : fields) {
        switch (field.type) {
            case ControlTableField::FieldType::UInt8: {
                auto value = data.uint8_at(field.addr);
                auto formatted_value = field.uint8.fmt(value);
                formatted_fields.emplace_back(field.name, std::move(formatted_value));
                break;
            }
            case ControlTableField::FieldType::UInt16: {
                auto value = data.uint16_at(field.addr);
                auto formatted_value = field.uint16.fmt(value);
                formatted_fields.emplace_back(field.name, std::move(formatted_value));
                break;
            }
            case ControlTableField::FieldType::UInt32: {
                auto value = data.uint32_at(field.addr);
                auto formatted_value = field.uint32.fmt(value);
                formatted_fields.emplace_back(field.name, std::move(formatted_value));
                break;
            }
        }
    }

    return formatted_fields;
}

enum class ProtocolResult {
    Ok,
    StatusIsInstruction,
    StatusHasError,
    InstructionIsStatus,
    UnknownInstruction,
    InvalidPacketLen,
    InvalidDeviceId,
    InvalidWrite,
    InvalidInstructionPacket,
};

class ControlTableMap {
  public:
    static const uint32_t MAX_ALLOWED_MISSED_PACKETS = 4;

    ControlTableMap();

    bool is_disconnected(DeviceId device_id) const;

    ProtocolResult receive(const Packet& packet);

    std::unordered_map<DeviceId, std::unique_ptr<ControlTable>>::const_iterator begin() const
        noexcept {
        return this->control_tables.begin();
    }

    std::unordered_map<DeviceId, std::unique_ptr<ControlTable>>::const_iterator end() const
        noexcept {
        return this->control_tables.end();
    }

  private:
    ProtocolResult receive_instruction_packet(const Packet& instruction_packet);

    ProtocolResult receive_status_packet(const Packet& status_packet);

    void register_control_table(DeviceId device_id, uint32_t model_number);

    std::unique_ptr<ControlTable>& get_control_table(DeviceId device_id);

    std::unordered_map<DeviceId, std::unique_ptr<ControlTable>> control_tables;
    std::unordered_map<DeviceId, uint32_t> num_missed_packets;
    InstructionPacket last_instruction_packet;
    bool is_last_instruction_packet_known;
    std::vector<DeviceId> pending_responses;
};

#endif
