#ifndef MX64_H
#define MX64_H

#include "control_table.h"
#include <stdint.h>

class Mx64ControlTable : public ControlTable {
  public:
    Mx64ControlTable();

    static const uint16_t MODEL_NUMBER = 311;

    static const std::vector<ControlTableField> FIELDS;

    std::unique_ptr<ControlTable> clone() const final {
        return std::make_unique<Mx64ControlTable>(*this);
    }

    uint16_t model_number() const final {
        return MODEL_NUMBER;
    }

    void set_firmware_version(uint8_t version) {
        this->mem.write_uint8(6, version);
    }

    const char* device_name() const final {
        return "MX-64";
    }

    ControlTableMemory& memory() final {
        return this->mem;
    }

    const ControlTableMemory& memory() const final {
        return this->mem;
    }

    const std::vector<ControlTableField>& fields() const final {
        return FIELDS;
    }

  private:
    ControlTableMemory mem;
};

#endif
