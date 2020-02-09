#ifndef MX106_H
#define MX106_H

#include "control_table.h"
#include <stdint.h>

class Mx106ControlTable : public ControlTable {
  public:
    Mx106ControlTable();

    static const uint16_t MODEL_NUMBER = 321;

    static const std::vector<ControlTableField> FIELDS;

    std::unique_ptr<ControlTable> clone() const final {
        return std::make_unique<Mx106ControlTable>(*this);
    }

    uint16_t model_number() const final {
        return MODEL_NUMBER;
    }

    void set_firmware_version(uint8_t version) {
        this->mem.write_uint8(6, version);
    }

    const char* device_name() const final {
        return "MX-106";
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
