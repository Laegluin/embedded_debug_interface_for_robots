#ifndef CORE_BOARD_H
#define CORE_BOARD_H

#include "control_table.h"

class CoreBoardControlTable : public ControlTable {
  public:
    CoreBoardControlTable();

    static const uint16_t MODEL_NUMBER = 0xabba;

    static const std::vector<ControlTableField> FIELDS;

    std::unique_ptr<ControlTable> clone() const final {
        return std::make_unique<CoreBoardControlTable>(*this);
    }

    uint16_t model_number() const final {
        return MODEL_NUMBER;
    }

    void set_firmware_version(uint8_t version) {
        this->mem.write_uint8(2, version);
    }

    const char* device_name() const final {
        return "Core";
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
