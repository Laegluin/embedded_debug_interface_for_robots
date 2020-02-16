#include "device/foot_pressure_sensor.h"
#include "device/fmt.h"

const uint16_t FootPressureSensorControlTable::MODEL_NUMBER;

const std::vector<ControlTableField> FootPressureSensorControlTable::FIELDS{
    ControlTableField::new_uint16(
        0,
        "Model Number",
        FootPressureSensorControlTable::MODEL_NUMBER,
        fmt_number),
    ControlTableField::new_uint8(2, "Firmware Version", 0, fmt_number),

    ControlTableField::new_uint32(36, "Front Left", 0, fmt_number),
    ControlTableField::new_uint32(40, "Front Right", 0, fmt_number),
    ControlTableField::new_uint32(44, "Back Left", 0, fmt_number),
    ControlTableField::new_uint32(48, "Back Right", 0, fmt_number),

};

FootPressureSensorControlTable::FootPressureSensorControlTable() :
    mem(ControlTableMemory({Segment::new_data(0, 52)})) {
    for (auto& field : FIELDS) {
        field.init_memory(this->mem);
    }
}
