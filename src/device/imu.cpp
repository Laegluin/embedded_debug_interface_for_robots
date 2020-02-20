#include "device/imu.h"
#include "device/fmt.h"

const uint16_t ImuControlTable::MODEL_NUMBER;

const std::vector<ControlTableField> ImuControlTable::FIELDS{
    ControlTableField::new_uint16(0, "Model Number", ImuControlTable::MODEL_NUMBER, fmt_number),
    ControlTableField::new_uint8(2, "Firmware Version", 0, fmt_number),

    ControlTableField::new_float32(36, "Acceleration X", 0, fmt_imu_accel),
    ControlTableField::new_float32(40, "Acceleration Y", 0, fmt_imu_accel),
    ControlTableField::new_float32(44, "Acceleration Z", 0, fmt_imu_accel),
    ControlTableField::new_float32(48, "Gyro X", 0, fmt_imu_gyro),
    ControlTableField::new_float32(52, "Gyro Y", 0, fmt_imu_gyro),
    ControlTableField::new_float32(56, "Gyro Z", 0, fmt_imu_gyro),
    ControlTableField::new_float32(60, "Orientation X", 0, fmt_number),
    ControlTableField::new_float32(64, "Orientation Y", 0, fmt_number),
    ControlTableField::new_float32(68, "Orientation Z", 0, fmt_number),
    ControlTableField::new_float32(72, "Orientation W", 0, fmt_number),
    ControlTableField::new_uint8(76, "Gyro Range", 3, fmt_imu_gyro_range),
    ControlTableField::new_uint8(77, "Acceleration Range", 3, fmt_imu_accel_range),
};

ImuControlTable::ImuControlTable() : mem(ControlTableMemory({Segment::new_data(0, 78)})) {
    for (auto& field : FIELDS) {
        field.init_memory(this->mem);
    }
}
