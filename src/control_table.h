#ifndef CONTROL_TABLE_H
#define CONTROL_TABLE_H

#include "endian_convert.h"
#include "parser.h"
#include <limits>
#include <memory>
#include <numeric>
#include <stdint.h>
#include <string.h>
#include <string>
#include <unordered_map>

class Segment {
  public:
    struct DataSegment {
        uint16_t start_addr;
        uint16_t len;
        uint8_t* data;
    };

    struct IndirectAddressSegment {
        uint16_t data_start_addr;
        uint16_t map_start_addr;
        uint16_t len;
        uint8_t* map;
    };

    enum class Type {
        DataSegment,
        IndirectAddressSegment,
    };

    static Segment new_data(uint16_t start_addr, uint16_t len);

    static Segment
        new_indirect_address(uint16_t data_start_addr, uint16_t map_start_addr, uint16_t len);

    uint16_t start_addr() const;

    uint16_t len() const;

    bool resolve_addr(uint16_t addr, uint16_t* resolved_addr) const;

    void set_backing_storage(uint8_t* buf);

    bool read(uint16_t addr, uint8_t* byte) const;

    bool write(uint16_t addr, uint8_t byte);

  private:
    Type type;
    union {
        DataSegment data;
        IndirectAddressSegment indirect_address;
    };
};

class ControlTableMemory {
  public:
    /// Creates a new `ControlTableMemory` consisting of the given `Segment`s.
    /// Segments must not overlap.
    ControlTableMemory(std::vector<Segment>&& segments);

    bool read_uint8(uint16_t addr, uint8_t* dst) const;

    bool read_uint16(uint16_t addr, uint16_t* dst) const;

    bool read_uint32(uint16_t addr, uint32_t* dst) const;

    bool read(uint16_t start_addr, uint8_t* dst, uint16_t len) const;

    bool write_uint8(uint16_t addr, uint8_t value);

    bool write_uint16(uint16_t addr, uint16_t value);

    bool write_uint32(uint16_t addr, uint32_t value);

    bool write(uint16_t start_addr, const uint8_t* buf, uint16_t len);

    uint16_t resolve_addr(uint16_t addr) const;

  private:
    std::vector<Segment> segments;
    std::vector<uint8_t> buf;
};

/// Represents a field in a control table. A field has an address and type, a name, a
/// default value and an optional formatting function that can format the field value as
/// a human readable string.
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

    /// Initializes the field in `mem` with the configured default value.
    void init_memory(ControlTableMemory& mem) const {
        switch (this->type) {
            case ControlTableField::FieldType::UInt8: {
                mem.write_uint8(this->addr, this->uint8.default_value);
                break;
            }
            case ControlTableField::FieldType::UInt16: {
                mem.write_uint16(this->addr, this->uint16.default_value);
                break;
            }
            case ControlTableField::FieldType::UInt32: {
                mem.write_uint32(this->addr, this->uint32.default_value);
                break;
            }
        }
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

class ControlTable {
  public:
    /// Tests if the model of the control table is known. If it is not know, the return value of
    /// `ControlTable::model_number` is undefined. This should not be overridden by device
    /// implementations.
    virtual bool is_unknown_model() const;

    /// Returns the model number of the device.
    virtual uint16_t model_number() const = 0;

    /// Sets the firmware version of the device (usually obtained through a ping response).
    virtual void set_firmware_version(uint8_t version) = 0;

    virtual const char* device_name() const = 0;

    virtual ControlTableMemory& memory() = 0;

    virtual const ControlTableMemory& memory() const = 0;

    virtual const std::vector<ControlTableField>& fields() const = 0;

    bool write(uint16_t start_addr, const uint8_t* buf, uint16_t len);

    std::vector<std::pair<const char*, std::string>> fmt_fields() const;
};

class UnknownControlTable : public ControlTable {
  public:
    UnknownControlTable() :
        mem(ControlTableMemory({Segment::new_data(0, 3)})),
        is_unknown_model_(true) {}

    UnknownControlTable(uint16_t model_number) :
        mem(ControlTableMemory({Segment::new_data(0, 3)})),
        is_unknown_model_(false) {
        this->mem.write_uint16(0, model_number);
    }

    bool is_unknown_model() const final {
        return is_unknown_model_;
    }

    uint16_t model_number() const final {
        uint16_t model_number;
        this->mem.read_uint16(0, &model_number);
        return model_number;
    }

    void set_firmware_version(uint8_t version) {
        this->mem.write_uint8(2, version);
    }

    const char* device_name() const final {
        return "<unknown>";
    }

    ControlTableMemory& memory() final {
        return this->mem;
    }

    const ControlTableMemory& memory() const final {
        return this->mem;
    }

    const std::vector<ControlTableField>& fields() const final {
        static std::vector<ControlTableField> fields{
            ControlTableField::new_uint16(
                0, "Model Number", 0, [](uint16_t number) { return std::to_string(number); }),
            ControlTableField::new_uint8(
                2, "Firmware Version", 0, [](uint8_t version) { return std::to_string(version); }),
        };

        static std::vector<ControlTableField> fields_no_model{
            ControlTableField::new_uint8(
                2, "Firmware Version", 0, [](uint8_t version) { return std::to_string(version); }),
        };

        if (this->is_unknown_model()) {
            return fields_no_model;
        } else {
            return fields;
        }
    }

  private:
    ControlTableMemory mem;
    bool is_unknown_model_;
};

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

std::string to_string(const ProtocolResult& result);

class ControlTableMap {
  public:
    static const uint32_t MAX_ALLOWED_MISSED_PACKETS = 4;

    ControlTableMap();

    bool is_disconnected(DeviceId device_id) const;

    /// Gets the `ControlTable` for `device_id`. Throws a `std::out_of_range` exception
    /// if the element device does not exist.
    const ControlTable& get(DeviceId device_id) const {
        return *this->control_tables.at(device_id);
    }

    ProtocolResult receive(const Packet& packet);

    size_t size() const {
        return this->control_tables.size();
    }

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

    ControlTable& register_control_table(DeviceId device_id, uint16_t model_number);

    /// Gets the `ControlTable` for `device_id` or inserts an unknown table if it does not exist.
    ControlTable& get_or_insert(DeviceId device_id);

    std::unordered_map<DeviceId, std::unique_ptr<ControlTable>> control_tables;
    std::unordered_map<DeviceId, uint32_t> num_missed_packets;
    InstructionPacket last_instruction_packet;
    bool is_last_instruction_packet_known;
    std::vector<DeviceId> pending_responses;
};

#endif
