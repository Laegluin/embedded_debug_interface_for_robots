#include "device/mx106.h"
#include "device/fmt.h"

// See <http://emanual.robotis.com/docs/en/dxl/mx/mx-106-2/>

static std::array<ControlTableField, 52> FIELDS{
    ControlTableField::new_uint16(0, "Model Number", Mx106ControlTable::MODEL_NUMBER, to_string),
    ControlTableField::new_uint32(2, "Model Information", 0, to_string),
    ControlTableField::new_uint8(6, "Firmware Version", 0, to_string),
    ControlTableField::new_uint8(7, "Id", 1, to_string),
    ControlTableField::new_uint8(8, "Baud Rate", 1, to_string),
    ControlTableField::new_uint8(9, "Return Delay Time", 250, to_string),
    ControlTableField::new_uint8(10, "Drive Mode", 0, to_string),
    ControlTableField::new_uint8(11, "Operating Mode", 3, to_string),
    ControlTableField::new_uint8(12, "Secondary Id", 255, to_string),
    ControlTableField::new_uint8(13, "Protocol Type", 2, to_string),
    ControlTableField::new_uint32(20, "Homing Offset", 0, to_string),
    ControlTableField::new_uint32(24, "Moving Threshold", 10, to_string),
    ControlTableField::new_uint8(31, "Temperature Limit", 80, to_string),
    ControlTableField::new_uint16(32, "Max Voltage Limit", 160, to_string),
    ControlTableField::new_uint16(34, "Min Voltage Limit", 95, to_string),
    ControlTableField::new_uint16(36, "PWM Limit", 885, to_string),
    ControlTableField::new_uint16(38, "Current Limit", 2047, to_string),
    ControlTableField::new_uint32(40, "Acceleration Limit", 32767, to_string),
    ControlTableField::new_uint32(44, "Velocity Limit", 210, to_string),
    ControlTableField::new_uint32(48, "Max Position Limit", 4095, to_string),
    ControlTableField::new_uint32(52, "Min Position Limit", 0, to_string),
    ControlTableField::new_uint8(63, "Shutdown", 52, to_string),
    ControlTableField::new_uint8(64, "Torque Enable", 0, to_string),
    ControlTableField::new_uint8(65, "LED", 0, to_string),
    ControlTableField::new_uint8(68, "Status Return Level", 2, to_string),
    ControlTableField::new_uint8(69, "Registered Instruction", 0, to_string),
    ControlTableField::new_uint8(70, "Hardware Error Status", 0, to_string),
    ControlTableField::new_uint16(76, "Velocity I-Gain", 1920, to_string),
    ControlTableField::new_uint16(78, "Velocity P-Gain", 100, to_string),
    ControlTableField::new_uint16(80, "Position D-Gain", 0, to_string),
    ControlTableField::new_uint16(82, "Position I-Gain", 0, to_string),
    ControlTableField::new_uint16(84, "Position P-Gain", 850, to_string),
    ControlTableField::new_uint16(88, "Feedforward 2nd Gain", 0, to_string),
    ControlTableField::new_uint16(90, "Feedforward 1st Gain", 0, to_string),
    ControlTableField::new_uint8(98, "Bus Watchdog", 0, to_string),
    ControlTableField::new_uint16(100, "Goal PWM", 0, to_string),
    ControlTableField::new_uint16(102, "Goal Current", 0, to_string),
    ControlTableField::new_uint32(104, "Goal Velocity", 0, to_string),
    ControlTableField::new_uint32(108, "Profile Acceleration", 0, to_string),
    ControlTableField::new_uint32(112, "Profile Velocity", 0, to_string),
    ControlTableField::new_uint32(116, "Goal Position", 0, to_string),
    ControlTableField::new_uint16(120, "Realtime Tick", 0, to_string),
    ControlTableField::new_uint8(122, "Moving", 0, to_string),
    ControlTableField::new_uint8(123, "Moving Status", 0, to_string),
    ControlTableField::new_uint16(124, "Present PWM", 0, to_string),
    ControlTableField::new_uint16(126, "Present Current", 0, to_string),
    ControlTableField::new_uint32(128, "Present Velocity", 0, to_string),
    ControlTableField::new_uint32(132, "Present Position", 0, to_string),
    ControlTableField::new_uint32(136, "Velocity Trajectory", 0, to_string),
    ControlTableField::new_uint32(140, "Position Trajectory", 0, to_string),
    ControlTableField::new_uint16(144, "Present Input Voltage", 0, to_string),
    ControlTableField::new_uint8(146, "Present Temperature", 0, to_string),
};

Mx106ControlTable::Mx106ControlTable() {
    default_init_control_table(this->data, FIELDS);

    this->addr_map_1.write_uint16(168, 224);
    this->addr_map_1.write_uint16(170, 225);
    this->addr_map_1.write_uint16(172, 226);
    this->addr_map_1.write_uint16(174, 227);
    this->addr_map_1.write_uint16(176, 228);
    this->addr_map_1.write_uint16(178, 229);
    this->addr_map_1.write_uint16(180, 230);
    this->addr_map_1.write_uint16(182, 231);
    this->addr_map_1.write_uint16(184, 232);
    this->addr_map_1.write_uint16(186, 233);
    this->addr_map_1.write_uint16(188, 234);
    this->addr_map_1.write_uint16(190, 235);
    this->addr_map_1.write_uint16(192, 236);
    this->addr_map_1.write_uint16(194, 237);
    this->addr_map_1.write_uint16(196, 238);
    this->addr_map_1.write_uint16(198, 239);
    this->addr_map_1.write_uint16(200, 240);
    this->addr_map_1.write_uint16(202, 241);
    this->addr_map_1.write_uint16(204, 242);
    this->addr_map_1.write_uint16(206, 243);
    this->addr_map_1.write_uint16(208, 244);
    this->addr_map_1.write_uint16(210, 245);
    this->addr_map_1.write_uint16(212, 246);
    this->addr_map_1.write_uint16(214, 247);
    this->addr_map_1.write_uint16(216, 248);
    this->addr_map_1.write_uint16(218, 249);
    this->addr_map_1.write_uint16(220, 250);
    this->addr_map_1.write_uint16(222, 251);

    this->addr_map_2.write_uint16(578, 634);
    this->addr_map_2.write_uint16(580, 635);
    this->addr_map_2.write_uint16(582, 636);
    this->addr_map_2.write_uint16(584, 637);
    this->addr_map_2.write_uint16(586, 638);
    this->addr_map_2.write_uint16(588, 639);
    this->addr_map_2.write_uint16(590, 640);
    this->addr_map_2.write_uint16(592, 641);
    this->addr_map_2.write_uint16(594, 642);
    this->addr_map_2.write_uint16(596, 643);
    this->addr_map_2.write_uint16(598, 644);
    this->addr_map_2.write_uint16(600, 645);
    this->addr_map_2.write_uint16(602, 646);
    this->addr_map_2.write_uint16(604, 647);
    this->addr_map_2.write_uint16(606, 648);
    this->addr_map_2.write_uint16(608, 649);
    this->addr_map_2.write_uint16(610, 650);
    this->addr_map_2.write_uint16(612, 651);
    this->addr_map_2.write_uint16(614, 652);
    this->addr_map_2.write_uint16(616, 653);
    this->addr_map_2.write_uint16(618, 654);
    this->addr_map_2.write_uint16(620, 655);
    this->addr_map_2.write_uint16(622, 656);
    this->addr_map_2.write_uint16(624, 657);
    this->addr_map_2.write_uint16(626, 658);
    this->addr_map_2.write_uint16(628, 659);
    this->addr_map_2.write_uint16(630, 660);
    this->addr_map_2.write_uint16(632, 661);
}

std::vector<std::pair<const char*, std::string>> Mx106ControlTable::fmt_fields() const {
    return fmt_control_table_fields(this->data, FIELDS);
}
