#ifndef CONTROL_TABLE_H
#define CONTROL_TABLE_H

#include "endian_convert.h"
#include <stdint.h>
#include <string.h>
#include <string>
#include <unordered_map>

class ControlTable {
  public:
    virtual const char* device_name() const = 0;

    virtual bool write(uint16_t start_addr, const uint8_t* bytes, uint16_t len) = 0;

    virtual void entries(std::unordered_map<const char*, std::string>& name_to_value) const = 0;
};

class UnknownControlTable : public ControlTable {
    const char* device_name() const final {
        return "<unknown>";
    }

    bool write(uint16_t, const uint8_t*, uint16_t) final {
        return false;
    }

    void entries(std::unordered_map<const char*, std::string>&) const final {}
};

class DisconnectedControlTable : public ControlTable {
    const char* device_name() const final {
        return "<disconnected>";
    }

    bool write(uint16_t, const uint8_t*, uint16_t) final {
        return false;
    }

    void entries(std::unordered_map<const char*, std::string>&) const final {}
};

template <uint16_t MAP_START, uint16_t DATA_START, uint16_t LEN>
class AddressMap {
  public:
    AddressMap() {
        memset(this->map, 0, LEN * 2);
    }

    bool write(uint16_t start_addr, const uint8_t* bytes, uint16_t len) {
        if (this->is_valid_map_addr(start_addr) && this->is_valid_map_addr(start_addr + len)) {
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

    bool is_valid_map_addr(uint16_t addr) const {
        return addr >= MAP_START && addr < MAP_START + LEN * 2;
    }

    bool is_valid_data_addr(uint16_t addr) const {
        return addr >= DATA_START && addr < DATA_START + LEN;
    }

  private:
    uint8_t map[LEN * 2];
};

template <uint16_t START, uint16_t LEN>
class DataSegment {
  public:
    DataSegment() {
        memset(this->data, 0, LEN);
    }

    bool write(uint16_t start_addr, const uint8_t* bytes, uint16_t len) {
        if (this->is_valid_addr(start_addr) && this->is_valid_addr(start_addr + len)) {
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

    bool is_valid_addr(uint16_t addr) const {
        return addr >= START && addr < START + LEN;
    }

  private:
    uint8_t data[LEN];
};

#endif