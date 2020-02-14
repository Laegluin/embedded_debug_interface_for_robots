#include "device/imu.h"
#include "device/fmt.h"

const uint16_t ImuControlTable::MODEL_NUMBER;

const std::vector<ControlTableField> ImuControlTable::FIELDS_1{
    ControlTableField::new_uint16(0, "Model Number", ImuControlTable::MODEL_NUMBER, to_string),
    ControlTableField::new_uint8(2, "Firmware Version", 0, to_string),

    ControlTableField::new_uint16(36, "Acceleration X", 0, to_string),
    ControlTableField::new_uint16(38, "Acceleration Y", 0, to_string),
    ControlTableField::new_uint16(40, "Acceleration Z", 0, to_string),
    ControlTableField::new_uint16(42, "Gyro X", 0, to_string),
    ControlTableField::new_uint16(44, "Gyro Y", 0, to_string),
    ControlTableField::new_uint16(46, "Gyro Z", 0, to_string),
    ControlTableField::new_uint32(48, "Sequence", 0, to_string),
};

const std::vector<ControlTableField> ImuControlTable::FIELDS_2{
    ControlTableField::new_uint16(0, "Model Number", ImuControlTable::MODEL_NUMBER, to_string),
    ControlTableField::new_uint8(2, "Firmware Version", 0, to_string),

    ControlTableField::new_uint16(52, "Acceleration X", 0, to_string),
    ControlTableField::new_uint16(54, "Acceleration Y", 0, to_string),
    ControlTableField::new_uint16(56, "Acceleration Z", 0, to_string),
    ControlTableField::new_uint16(58, "Gyro X", 0, to_string),
    ControlTableField::new_uint16(60, "Gyro Y", 0, to_string),
    ControlTableField::new_uint16(62, "Gyro Z", 0, to_string),
    ControlTableField::new_uint32(64, "Sequence", 0, to_string),
};

ImuControlTable::ImuControlTable() : mem(ControlTableMemory({Segment::new_data(0, 68)})) {
    for (auto& field : FIELDS_1) {
        field.init_memory(this->mem);
    }

    for (auto& field : FIELDS_2) {
        field.init_memory(this->mem);
    }
}

const std::vector<ControlTableField>& ImuControlTable::fields() const {
    uint32_t seq_1;
    this->mem.read_uint32(48, &seq_1);

    uint32_t seq_2;
    this->mem.read_uint32(64, &seq_2);

    // The IMU actually always holds two measurements for each value.
    // Both batches also come with a sequence number that can be used
    // to determine the most recent measurement. For writes we simply
    // represent the whole address space, so we must look at the sequence
    // number to return the correct addresses for the up to date values.
    if (seq_1 > seq_2) {
        return FIELDS_1;
    } else {
        return FIELDS_2;
    }
}
