#ifndef PARSER_H
#define PARSER_H

/// See <http://emanual.robotis.com/docs/en/dxl/protocol2/>

#include "buffer.h"
#include <array>
#include <option.h>
#include <stddef.h>
#include <stdint.h>

const uint32_t PACKET_BUF_LEN = 256;

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

  private:
    uint8_t id;
};

inline bool operator==(const DeviceId& lhs, const DeviceId& rhs) {
    return lhs.id == rhs.id;
}

inline bool operator!=(const DeviceId& lhs, const DeviceId& rhs) {
    return !(lhs == rhs);
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
    explicit Error(uint8_t code) : code(code) {}

    bool is_ok() const {
        return this->code == 0;
    }

    bool is_alert() const {
        return (this->code & 0b01000000) != 0;
    }

  private:
    uint8_t code;
};

class Packet {
  public:
    DeviceId device_id;
    Instruction instruction;
    Error error;
    FixedByteVector<PACKET_BUF_LEN> data;
};

class Receiver {
  public:
    Receiver() : last_bytes({0, 0, 0}), crc(0) {}

    bool wait_for_header(Cursor& cursor);

    size_t read(Cursor& cursor, uint8_t* dst, size_t num_bytes);

    size_t read_raw_num_bytes(Cursor& cursor, uint8_t* dst, size_t* dst_len, size_t raw_num_bytes);

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

enum class ParseError {
    BufferOverflow,
    MismatchedChecksum,
};

class ParseResult {
  public:
    enum class Result {
        PacketAvailable,
        NeedMoreData,
        Error,
    };

    ParseResult(ParseError error) : result(Result::Error), error(error) {}

    static ParseResult packet_available() {
        return ParseResult(Result::PacketAvailable);
    }

    static ParseResult need_more_data() {
        return ParseResult(Result::NeedMoreData);
    }

  private:
    ParseResult(Result result) : result(result) {}

    Result result;
    ParseError error;
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
    Parser() : current_state(ParserState::Header), raw_remaining_data_len(0) {}

    ParseResult parse(Cursor& cursor, Packet& packet);

  private:
    // only need this for storing partial reads that are not data (data is
    // stored in the packet directly)
    FixedByteVector<4> buf;
    Receiver receiver;
    ParserState current_state;
    size_t raw_remaining_data_len;
};

#endif
