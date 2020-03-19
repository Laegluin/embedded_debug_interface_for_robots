#ifndef CONTROL_TABLE_H
#define CONTROL_TABLE_H

#include "device_id_map.h"
#include "endian_convert.h"
#include "parser.h"
#include <limits>
#include <memory>
#include <numeric>
#include <stdint.h>
#include <string.h>
#include <string>

/// Describes a segment of memory in a device's `ControlTableMemory`.
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
        Unknown,
    };

    /// Creates a new `Segment` that simply stores `len` bytes of data starting at `start_addr`.
    static Segment new_data(uint16_t start_addr, uint16_t len);

    /// Creates a new `Segment` that stores addresses starting at `map_start_addr`. Addresses
    /// starting at `data_start_addr` and ending at `data_start_addr + len` are resolved to
    /// these addresses.
    static Segment
        new_indirect_address(uint16_t data_start_addr, uint16_t map_start_addr, uint16_t len);

    /// Creates a new `Segment` that accepts any writes and does not allow any reads.
    static Segment new_unknown();

    uint16_t start_addr() const;

    uint16_t len() const;

    bool resolve_addr(uint16_t addr, uint16_t* resolved_addr) const;

    /// Sets the buffer that is used to store the data for this `Segment`. Exactly `len` bytes
    /// will be used. This method is called by the `ControlTableMemory` object the segment
    /// belongs to.
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

/// Stores all data of a control table. The memory consists of one or more `Segment`s that
/// describe at which addresses values are stored. Segments may overlap but the first one
/// that matches a given address is always used.
class ControlTableMemory {
  public:
    /// Creates a new `ControlTableMemory` consisting of the given `Segment`s.
    /// Segments must not overlap.
    ControlTableMemory(std::vector<Segment>&& segments);

    ControlTableMemory(const ControlTableMemory& src);

    bool read_uint8(uint16_t addr, uint8_t* dst) const;

    bool read_uint16(uint16_t addr, uint16_t* dst) const;

    bool read_uint32(uint16_t addr, uint32_t* dst) const;

    bool read_float32(uint16_t addr, float* dst) const;

    bool read(uint16_t start_addr, uint8_t* dst, uint16_t len) const;

    bool write_uint8(uint16_t addr, uint8_t value);

    bool write_uint16(uint16_t addr, uint16_t value);

    bool write_uint32(uint16_t addr, uint32_t value);

    bool write_float32(uint16_t addr, float value);

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
        Float32,
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

    static ControlTableField new_float32(
        uint16_t addr,
        const char* name,
        float default_value,
        std::string (*fmt)(float)) {
        ControlTableField field;
        field.addr = addr;
        field.type = FieldType::Float32;
        field.name = name;
        field.float32.default_value = default_value;
        field.float32.fmt = fmt;
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
            case ControlTableField::FieldType::Float32: {
                mem.write_float32(this->addr, this->float32.default_value);
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
        struct {
            float default_value;
            std::string (*fmt)(float);
        } float32;
    };
};

/// Represents the control table of a single device. This is where data from packets is stored.
class ControlTable {
  public:
    virtual ~ControlTable() = default;

    /// Clones the control table (calls the correct copy constructor).
    virtual std::unique_ptr<ControlTable> clone() const = 0;

    /// Tests if the model of the control table is known. If it is not know, the return value of
    /// `ControlTable::model_number` is undefined. This should not be overridden by device
    /// implementations.
    virtual bool is_unknown_model() const;

    /// Returns the model number of the device.
    virtual uint16_t model_number() const = 0;

    /// Sets the firmware version of the device (usually obtained through a ping response).
    virtual void set_firmware_version(uint8_t version) = 0;

    /// Returns the human readable name for the model of the device.
    virtual const char* device_name() const = 0;

    /// Returns a reference to the control table's memory.
    virtual ControlTableMemory& memory() = 0;

    /// Returns a reference to the control table's memory.
    virtual const ControlTableMemory& memory() const = 0;

    /// Returns a reference to the field definitions of the control table.
    virtual const std::vector<ControlTableField>& fields() const = 0;

    /// Writes `len` bytes from `buf` to the control table, starting at `start_addr`.
    /// Returns `false` if parts of the write were not in bounds. In this case, some data
    /// may have been written already.
    bool write(uint16_t start_addr, const uint8_t* buf, uint16_t len);

    /// Returns pairs of field name and value for every field of the control table. The values
    /// are formatted using the formatting function specified by the field definition. They are
    /// read from the `ControlTableMemory` returned by the `memory` method.
    std::vector<std::pair<const char*, std::string>> fmt_fields() const;
};

/// Represents the control table of an unknown or unsupported device model. Ignores any writes to
/// it but does not return an error.
class UnknownControlTable : public ControlTable {
  public:
    /// Creates the control table for an unknown model. This can happen when the no ping
    /// instructions for the device have been received.
    UnknownControlTable() :
        mem(ControlTableMemory({
            Segment::new_data(0, 3),
            Segment::new_unknown(),
        })),
        is_unknown_model_(true) {}

    /// Creates the control table for an unsupported device model. `model_number` is the
    /// model number that was part of the response to a ping instruction.
    UnknownControlTable(uint16_t model_number) :
        mem(ControlTableMemory({
            Segment::new_data(0, 3),
            Segment::new_unknown(),
        })),
        is_unknown_model_(false) {
        this->mem.write_uint16(0, model_number);
    }

    std::unique_ptr<ControlTable> clone() const final {
        return std::make_unique<UnknownControlTable>(*this);
    }

    bool is_unknown_model() const final {
        return is_unknown_model_;
    }

    uint16_t model_number() const final {
        uint16_t model_number = 0;
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

/// Result returned when processing packets.
enum class ProtocolResult {
    Ok,

    /// Packet is an instruction packet but a status packet was expected.
    StatusIsInstruction,

    /// The status packet has an error set in its error field.
    StatusHasError,

    /// Packet is a status packet but an instruction packet was expected.
    InstructionIsStatus,

    /// The packet contains an unknown instruction.
    UnknownInstruction,

    /// The packet length is not valid for the current instruction/status packet.
    InvalidPacketLen,

    /// The device id is not valid for the packet.
    InvalidDeviceId,

    /// A packet contained data that is not in bounds of the device's control table.
    InvalidWrite,

    /// The payload of an instruction packet could not be parsed.
    InvalidInstructionPacket,
};

std::string to_string(const ProtocolResult& result);

/// Maps `DeviceId`s to `ControlTable`s. The control tables are updated with every
/// call to `receive`.
class ControlTableMap {
  public:
    /// The number of times a device is allowed to not respond before it is
    /// considered disconnected.
    static const uint32_t MAX_ALLOWED_MISSED_PACKETS = 4;

    ControlTableMap();

    /// Determines if the device identified by `device_id` is disconnected. If
    /// the device was not encountered before, `false` is returned.
    bool is_disconnected(DeviceId device_id) const;

    /// Gets the `ControlTable` entry for `device_id`.
    const DeviceIdMap<std::unique_ptr<ControlTable>>::Entry& get(DeviceId device_id) const {
        return this->control_tables.get(device_id);
    }

    /// Processes the next packet and updates the control tables and disconnected state
    /// of the affected devices.
    ProtocolResult receive(const Packet& packet);

    size_t size() const {
        return this->control_tables.size();
    }

    DeviceIdMap<std::unique_ptr<ControlTable>>::const_iterator begin() const noexcept {
        return this->control_tables.begin();
    }

    DeviceIdMap<std::unique_ptr<ControlTable>>::const_iterator end() const noexcept {
        return this->control_tables.end();
    }

  private:
    ProtocolResult receive_instruction_packet(const Packet& instruction_packet);

    ProtocolResult receive_status_packet(const Packet& status_packet);

    /// Creates the correct control table for the given `model_number` and inserts it for
    /// the `device_id`. If a control table already existed for the id it is only replaced
    /// if the model number is different or unknown.
    ControlTable& register_control_table(DeviceId device_id, uint16_t model_number);

    /// Gets the `ControlTable` for `device_id` or inserts an unknown table if it does not exist.
    ControlTable& get_or_insert(DeviceId device_id);

    DeviceIdMap<std::unique_ptr<ControlTable>> control_tables;
    DeviceIdMap<uint32_t> num_missed_packets;
    InstructionPacket last_instruction_packet;
    bool is_last_instruction_packet_known;
    std::vector<DeviceId> pending_responses;
};

#endif
