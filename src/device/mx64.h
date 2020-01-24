#ifndef MX64_H
#define MX64_H

#include "control_table.h"
#include <stdint.h>

class Mx64ControlTable : public ControlTable {
  public:
    Mx64ControlTable();

    static const uint32_t MODEL_NUMBER = 311;

    static const std::vector<ControlTableField> FIELDS;

    uint32_t model_number() const final {
        return MODEL_NUMBER;
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
