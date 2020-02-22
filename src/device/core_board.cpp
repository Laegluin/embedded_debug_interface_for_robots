#include "device/core_board.h"
#include "device/fmt.h"

const uint16_t CoreBoardControlTable::MODEL_NUMBER;

const std::vector<ControlTableField> CoreBoardControlTable::FIELDS{
    ControlTableField::new_uint16(
        0,
        "Model Number",
        CoreBoardControlTable::MODEL_NUMBER,
        fmt_number),
    ControlTableField::new_uint8(2, "Firmware Version", 0, fmt_number),

    ControlTableField::new_uint16(10, "LED", 0, fmt_bool_on_off),
    ControlTableField::new_uint16(12, "Power", 0, fmt_number),
    ControlTableField::new_uint32(14, "RGB LED 1", 0, fmt_core_rgb),
    ControlTableField::new_uint32(18, "RGB LED 2", 0, fmt_core_rgb),
    ControlTableField::new_uint32(22, "RGB LED 3", 0, fmt_core_rgb),
    ControlTableField::new_uint16(26, "VBAT", 0, fmt_core_voltage),
    ControlTableField::new_uint16(28, "VEXT", 0, fmt_core_voltage),
    ControlTableField::new_uint16(30, "VCC", 0, fmt_core_voltage),
    ControlTableField::new_uint16(32, "VDXL", 0, fmt_core_voltage),
    ControlTableField::new_uint16(34, "Current", 0, fmt_core_current),
    ControlTableField::new_uint16(36, "Power On", 0, fmt_core_power_on),
};

CoreBoardControlTable::CoreBoardControlTable() :
    mem(ControlTableMemory({Segment::new_data(0, 38)})) {
    for (auto& field : FIELDS) {
        field.init_memory(this->mem);
    }
}
