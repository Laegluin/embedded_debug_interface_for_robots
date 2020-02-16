#include "device/mx106.h"
#include "device/fmt.h"

// See <http://emanual.robotis.com/docs/en/dxl/mx/mx-106-2/>

const uint16_t Mx106ControlTable::MODEL_NUMBER;

const std::vector<ControlTableField> Mx106ControlTable::FIELDS{
    ControlTableField::new_uint16(0, "Model Number", Mx106ControlTable::MODEL_NUMBER, fmt_number),
    ControlTableField::new_uint32(2, "Model Information", 0, fmt_number),
    ControlTableField::new_uint8(6, "Firmware Version", 0, fmt_number),
    ControlTableField::new_uint8(7, "Id", 1, fmt_number),
    ControlTableField::new_uint8(8, "Baud Rate", 1, fmt_mx_baud_rate),
    ControlTableField::new_uint8(9, "Return Delay Time", 250, fmt_mx_return_delay),
    ControlTableField::new_uint8(10, "Drive Mode", 0, fmt_mx_drive_mode),
    ControlTableField::new_uint8(11, "Operating Mode", 3, fmt_mx_operating_mode),
    ControlTableField::new_uint8(12, "Secondary Id", 255, fmt_number),
    ControlTableField::new_uint8(13, "Protocol Type", 2, fmt_number),
    ControlTableField::new_uint32(20, "Homing Offset", 0, fmt_mx_homing_offset),
    ControlTableField::new_uint32(24, "Moving Threshold", 10, fmt_mx_moving_threshold),
    ControlTableField::new_uint8(31, "Temperature Limit", 80, fmt_mx_temp_limit),
    ControlTableField::new_uint16(32, "Max Voltage Limit", 160, fmt_mx_voltage_limit),
    ControlTableField::new_uint16(34, "Min Voltage Limit", 95, fmt_mx_voltage_limit),
    ControlTableField::new_uint16(36, "PWM Limit", 885, fmt_mx_pwm_limit),
    ControlTableField::new_uint16(38, "Current Limit", 2047, fmt_mx_current_limit),
    ControlTableField::new_uint32(40, "Acceleration Limit", 32767, fmt_mx_accel_limit),
    ControlTableField::new_uint32(44, "Velocity Limit", 210, fmt_mx_velocity_limit),
    ControlTableField::new_uint32(48, "Max Position Limit", 4095, fmt_mx_position_limit),
    ControlTableField::new_uint32(52, "Min Position Limit", 0, fmt_mx_position_limit),
    ControlTableField::new_uint8(63, "Shutdown", 52, fmt_mx_shutdown),
    ControlTableField::new_uint8(64, "Torque Enable", 0, fmt_bool),
    ControlTableField::new_uint8(65, "LED", 0, fmt_bool_on_off),
    ControlTableField::new_uint8(68, "Status Return Level", 2, fmt_mx_status_return),
    ControlTableField::new_uint8(69, "Registered Instruction", 0, fmt_bool),
    ControlTableField::new_uint8(70, "Hardware Error Status", 0, fmt_mx_hardware_error),
    ControlTableField::new_uint16(76, "Velocity I-Gain", 1920, fmt_mx_velocity_i_gain),
    ControlTableField::new_uint16(78, "Velocity P-Gain", 100, fmt_mx_velocity_p_gain),
    ControlTableField::new_uint16(80, "Position D-Gain", 0, fmt_mx_pos_d_gain),
    ControlTableField::new_uint16(82, "Position I-Gain", 0, fmt_mx_pos_i_gain),
    ControlTableField::new_uint16(84, "Position P-Gain", 850, fmt_mx_pos_p_gain),
    ControlTableField::new_uint16(88, "Feedforward 2nd Gain", 0, fmt_mx_ff_2nd_gain),
    ControlTableField::new_uint16(90, "Feedforward 1st Gain", 0, fmt_mx_ff_1st_gain),
    ControlTableField::new_uint8(98, "Bus Watchdog", 0, fmt_mx_watchdog),
    ControlTableField::new_uint16(100, "Goal PWM", 0, fmt_mx_goal_pwm),
    ControlTableField::new_uint16(102, "Goal Current", 0, fmt_mx_goal_current),
    ControlTableField::new_uint32(104, "Goal Velocity", 0, fmt_mx_goal_velocity),
    ControlTableField::new_uint32(108, "Profile Acceleration", 0, fmt_number),
    ControlTableField::new_uint32(112, "Profile Velocity", 0, fmt_mx_profile_velocity),
    ControlTableField::new_uint32(116, "Goal Position", 0, fmt_mx_goal_position),
    ControlTableField::new_uint16(120, "Realtime Tick", 0, fmt_mx_tick),
    ControlTableField::new_uint8(122, "Moving", 0, fmt_bool),
    ControlTableField::new_uint8(123, "Moving Status", 0, fmt_mx_moving_status),
    ControlTableField::new_uint16(124, "Present PWM", 0, fmt_mx_present_pwm),
    ControlTableField::new_uint16(126, "Present Current", 0, fmt_mx_present_current),
    ControlTableField::new_uint32(128, "Present Velocity", 0, fmt_mx_present_velocity),
    ControlTableField::new_uint32(132, "Present Position", 0, fmt_mx_present_position),
    ControlTableField::new_uint32(136, "Velocity Trajectory", 0, fmt_number),
    ControlTableField::new_uint32(140, "Position Trajectory", 0, fmt_number),
    ControlTableField::new_uint16(144, "Present Input Voltage", 0, fmt_mx_present_voltage),
    ControlTableField::new_uint8(146, "Present Temperature", 0, fmt_mx_present_temp),

    ControlTableField::new_uint16(168, "Indirect Address 1", 224, fmt_addr),
    ControlTableField::new_uint16(170, "Indirect Address 2", 225, fmt_addr),
    ControlTableField::new_uint16(172, "Indirect Address 3", 226, fmt_addr),
    ControlTableField::new_uint16(174, "Indirect Address 4", 227, fmt_addr),
    ControlTableField::new_uint16(176, "Indirect Address 5", 228, fmt_addr),
    ControlTableField::new_uint16(178, "Indirect Address 6", 229, fmt_addr),
    ControlTableField::new_uint16(180, "Indirect Address 7", 230, fmt_addr),
    ControlTableField::new_uint16(182, "Indirect Address 8", 231, fmt_addr),
    ControlTableField::new_uint16(184, "Indirect Address 9", 232, fmt_addr),
    ControlTableField::new_uint16(186, "Indirect Address 10", 233, fmt_addr),
    ControlTableField::new_uint16(188, "Indirect Address 11", 234, fmt_addr),
    ControlTableField::new_uint16(190, "Indirect Address 12", 235, fmt_addr),
    ControlTableField::new_uint16(192, "Indirect Address 13", 236, fmt_addr),
    ControlTableField::new_uint16(194, "Indirect Address 14", 237, fmt_addr),
    ControlTableField::new_uint16(196, "Indirect Address 15", 238, fmt_addr),
    ControlTableField::new_uint16(198, "Indirect Address 16", 239, fmt_addr),
    ControlTableField::new_uint16(200, "Indirect Address 17", 240, fmt_addr),
    ControlTableField::new_uint16(202, "Indirect Address 18", 241, fmt_addr),
    ControlTableField::new_uint16(204, "Indirect Address 19", 242, fmt_addr),
    ControlTableField::new_uint16(206, "Indirect Address 20", 243, fmt_addr),
    ControlTableField::new_uint16(208, "Indirect Address 21", 244, fmt_addr),
    ControlTableField::new_uint16(210, "Indirect Address 22", 245, fmt_addr),
    ControlTableField::new_uint16(212, "Indirect Address 23", 246, fmt_addr),
    ControlTableField::new_uint16(214, "Indirect Address 24", 247, fmt_addr),
    ControlTableField::new_uint16(216, "Indirect Address 25", 248, fmt_addr),
    ControlTableField::new_uint16(218, "Indirect Address 26", 249, fmt_addr),
    ControlTableField::new_uint16(220, "Indirect Address 27", 250, fmt_addr),
    ControlTableField::new_uint16(222, "Indirect Address 28", 251, fmt_addr),

    ControlTableField::new_uint16(578, "Indirect Address 29", 634, fmt_addr),
    ControlTableField::new_uint16(580, "Indirect Address 30", 635, fmt_addr),
    ControlTableField::new_uint16(582, "Indirect Address 31", 636, fmt_addr),
    ControlTableField::new_uint16(584, "Indirect Address 32", 637, fmt_addr),
    ControlTableField::new_uint16(586, "Indirect Address 33", 638, fmt_addr),
    ControlTableField::new_uint16(588, "Indirect Address 34", 639, fmt_addr),
    ControlTableField::new_uint16(590, "Indirect Address 35", 640, fmt_addr),
    ControlTableField::new_uint16(592, "Indirect Address 36", 641, fmt_addr),
    ControlTableField::new_uint16(594, "Indirect Address 37", 642, fmt_addr),
    ControlTableField::new_uint16(596, "Indirect Address 38", 643, fmt_addr),
    ControlTableField::new_uint16(598, "Indirect Address 39", 644, fmt_addr),
    ControlTableField::new_uint16(600, "Indirect Address 40", 645, fmt_addr),
    ControlTableField::new_uint16(602, "Indirect Address 41", 646, fmt_addr),
    ControlTableField::new_uint16(604, "Indirect Address 42", 647, fmt_addr),
    ControlTableField::new_uint16(606, "Indirect Address 43", 648, fmt_addr),
    ControlTableField::new_uint16(608, "Indirect Address 44", 649, fmt_addr),
    ControlTableField::new_uint16(610, "Indirect Address 45", 650, fmt_addr),
    ControlTableField::new_uint16(612, "Indirect Address 46", 651, fmt_addr),
    ControlTableField::new_uint16(614, "Indirect Address 47", 652, fmt_addr),
    ControlTableField::new_uint16(616, "Indirect Address 48", 653, fmt_addr),
    ControlTableField::new_uint16(618, "Indirect Address 49", 654, fmt_addr),
    ControlTableField::new_uint16(620, "Indirect Address 50", 655, fmt_addr),
    ControlTableField::new_uint16(622, "Indirect Address 51", 656, fmt_addr),
    ControlTableField::new_uint16(624, "Indirect Address 52", 657, fmt_addr),
    ControlTableField::new_uint16(626, "Indirect Address 53", 658, fmt_addr),
    ControlTableField::new_uint16(628, "Indirect Address 54", 659, fmt_addr),
    ControlTableField::new_uint16(630, "Indirect Address 55", 660, fmt_addr),
    ControlTableField::new_uint16(632, "Indirect Address 56", 661, fmt_addr),
};

Mx106ControlTable::Mx106ControlTable() :
    mem(ControlTableMemory(std::vector<Segment>{
        Segment::new_data(0, 147),
        Segment::new_indirect_address(224, 168, 56),
        Segment::new_indirect_address(634, 578, 56),
    })) {
    for (auto& field : FIELDS) {
        field.init_memory(this->mem);
    }
}
