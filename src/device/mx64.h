#ifndef MX64_H
#define MX64_H

#include "control_table.h"
#include <stdint.h>

class Mx64ControlTable : public ControlTable {
  public:
    Mx64ControlTable();

    static const uint32_t MODEL_NUMBER = 311;

    uint32_t model_number() const final {
        return MODEL_NUMBER;
    }

    const char* device_name() const final {
        return "MX-64";
    }

    bool write(uint16_t start_addr, const uint8_t* bytes, uint16_t len) final {
        auto resolved_addr =
            this->addr_map_2.resolve_addr(this->addr_map_1.resolve_addr(start_addr));

        return this->data.write(resolved_addr, bytes, len)
            || this->addr_map_1.write(resolved_addr, bytes, len)
            || this->addr_map_2.write(resolved_addr, bytes, len);
    }

    void entries(std::unordered_map<const char*, std::string>& name_to_value) const final;

  private:
    DataSegment<0, 147> data;
    AddressMap<168, 224, 28> addr_map_1;
    AddressMap<578, 634, 28> addr_map_2;
};

#endif
