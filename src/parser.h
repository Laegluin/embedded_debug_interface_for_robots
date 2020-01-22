#ifndef PARSER_H
#define PARSER_H

/// See <http://emanual.robotis.com/docs/en/dxl/protocol2/>

#include "cursor.h"
#include <array>
#include <stddef.h>
#include <stdint.h>
#include <vector>

const uint32_t MAX_PACKET_DATA_LEN = 256;

class DeviceId {
  public:
    explicit DeviceId(uint8_t id) : id(id) {}

    static DeviceId broadcast() {
        return DeviceId(254);
    }

    bool is_broadcast() const {
        return *this == DeviceId::broadcast();
    }

    friend bool operator==(const DeviceId& lhs, const DeviceId& rhs);

    friend class std::hash<DeviceId>;

    friend std::ostream& operator<<(std::ostream& out, const DeviceId& device_id);

  private:
    uint8_t id;
};

inline bool operator==(const DeviceId& lhs, const DeviceId& rhs) {
    return lhs.id == rhs.id;
}

inline bool operator!=(const DeviceId& lhs, const DeviceId& rhs) {
    return !(lhs == rhs);
}

namespace std {
    template <>
    struct hash<DeviceId> {
        size_t operator()(const DeviceId& device_id) const {
            return std::hash<uint8_t>()(device_id.id);
        }
    };
}

inline std::ostream& operator<<(std::ostream& out, const DeviceId& device_id) {
    out << std::to_string(device_id.id);
    return out;
}

enum class Instruction : uint8_t {
    Ping = 0x01,
    Read = 0x02,
    Write = 0x03,
    RegWrite = 0x04,
    Action = 0x05,
    FactoryReset = 0x06,
    Reboot = 0x08,
    Clear = 0x10,
    Status = 0x55,
    SyncRead = 0x82,
    SyncWrite = 0x83,
    BulkRead = 0x92,
    BulkWrite = 0x93,
};

class Error {
  public:
    Error() : code(0) {}

    explicit Error(uint8_t code) : code(code) {}

    bool is_ok() const {
        return this->code == 0;
    }

    bool is_alert() const {
        return (this->code & 0b01000000) != 0;
    }

    friend bool operator==(const Error& lhs, const Error& rhs);

  private:
    uint8_t code;
};

inline bool operator==(const Error& lhs, const Error& rhs) {
    return lhs.code == rhs.code;
}

struct Packet {
    DeviceId device_id;
    Instruction instruction;
    Error error;
    std::vector<uint8_t> data;
};

class Receiver {
  public:
    Receiver() : last_bytes({0, 0, 0}), crc(0) {}

    bool wait_for_header(Cursor& cursor);

    /// Reads at most `num_bytes` from `cursor` into `dst` and returns the number of bytes read.
    /// Bytes are counted and returned after stuffing has been removed. All read bytes (including
    /// stuffing are used to update the crc checksum).
    size_t read(Cursor& cursor, uint8_t* dst, size_t num_bytes);

    /// Like `Receiver::read` but uses and returns the number of bytes before stuffing has been
    /// removed. The bytes written to `dst` have all stuffing removed and the length of `dst` is
    /// written to `dst_len`.
    size_t read_raw_num_bytes(Cursor& cursor, uint8_t* dst, size_t* dst_len, size_t raw_num_bytes);

    /// Reads `num_bytes` from `cursor` into `dst` and returns the number of bytes read. Does not
    /// remove stuffing. __Does not update the crc checksum__.
    size_t read_raw(Cursor& cursor, uint8_t* dst, size_t num_bytes);

    uint16_t current_crc() const;

  private:
    bool is_stuffing_byte(uint8_t byte) const;

    void push_last_byte(uint8_t byte);

    void reset_crc();

    void update_crc(uint8_t byte);

    std::array<uint8_t, 3> last_bytes;
    uint16_t crc;
};

enum class ParseResult {
    PacketAvailable,
    NeedMoreData,
    BufferOverflow,
    MismatchedChecksum,
};

enum class ParserState {
    Header,
    CommonFields,
    ErrorField,
    Data,
    Checksum,
};

class Parser {
  public:
    Parser() : buf_len(0), current_state(ParserState::Header), raw_remaining_data_len(0) {}

    ParseResult parse(Cursor& cursor, Packet& packet);

  private:
    // only need this for storing partial reads that are not data (data is
    // stored in the packet directly)
    uint8_t buf[4];
    size_t buf_len;
    Receiver receiver;
    ParserState current_state;
    size_t raw_remaining_data_len;
};

struct PingArgs {
    DeviceId device_id;
};

struct ReadArgs {
    DeviceId device_id;
    uint16_t start_addr;
    uint16_t len;
};

struct WriteArgs {
    DeviceId device_id;
    uint16_t start_addr;
    std::vector<uint8_t> data;
};

enum class FactoryReset : uint8_t {
    ResetAll = 0xff,
    ResetAllExceptId = 0x01,
    ResetAllExceptIdAndBaudRate = 0x02,
};

struct FactoryResetArgs {
    DeviceId device_id;
    FactoryReset reset;
};

struct SyncReadArgs {
    std::vector<DeviceId> devices;
    uint16_t start_addr;
    uint16_t len;
};

struct SyncWriteArgs {
    std::vector<DeviceId> devices;
    uint16_t start_addr;
    uint16_t len;
    std::vector<uint8_t> data;
};

struct BulkReadArgs {
    std::vector<ReadArgs> reads;
};

struct BulkWriteArgs {
    std::vector<WriteArgs> writes;
};

struct InstructionPacket {
    InstructionPacket() : instruction(Instruction::Ping), ping(PingArgs{DeviceId(0)}) {}

    InstructionPacket(const InstructionPacket& src);

    ~InstructionPacket();

    Instruction instruction;
    union {
        PingArgs ping;
        ReadArgs read;
        WriteArgs write;
        WriteArgs reg_write;
        FactoryResetArgs factory_reset;
        SyncReadArgs sync_read;
        SyncWriteArgs sync_write;
        BulkReadArgs bulk_read;
        BulkWriteArgs bulk_write;
    };
};

enum class InstructionParseResult {
    Ok,
    InvalidPacketLen,
    InvalidDeviceId,
    InstructionIsStatus,
    UnknownInstruction,
};

InstructionParseResult
    parse_instruction_packet(const Packet& packet, InstructionPacket* instruction_packet);

#endif
