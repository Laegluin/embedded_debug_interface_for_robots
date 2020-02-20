#ifndef FMT_H
#define FMT_H

#include <bitset>
#include <iomanip>
#include <sstream>
#include <string>

inline std::string fmt_number(uint8_t value) {
    return std::to_string(value);
}

inline std::string fmt_number(uint16_t value) {
    return std::to_string(value);
}

inline std::string fmt_number(uint32_t value) {
    return std::to_string(value);
}

inline std::string fmt_addr(uint16_t addr) {
    std::stringstream fmt;
    fmt << "0x" << std::hex << std::setw(4) << std::setfill('0') << addr << " (" << std::dec << addr
        << ")";
    return fmt.str();
}

inline std::string fmt_bool(uint8_t value) {
    if (value) {
        return "true";
    } else {
        return "false";
    }
}

inline std::string fmt_bool(uint16_t value) {
    if (value) {
        return "true";
    } else {
        return "false";
    }
}

inline std::string fmt_bool_on_off(uint8_t value) {
    if (value) {
        return "on";
    } else {
        return "off";
    }
}

inline std::string fmt_core_rgb(uint32_t rgb) {
    std::stringstream fmt;
    fmt << "#" << std::setw(6) << std::setfill('0') << std::hex << rgb;
    return fmt.str();
}

inline std::string fmt_core_voltage(uint16_t raw) {
    std::stringstream fmt;
    float voltage = raw / 1024.0f * 3.3f * 6;
    fmt << voltage << " V";
    return fmt.str();
}

inline std::string fmt_core_current(uint16_t raw) {
    std::stringstream fmt;
    float current = raw / 1024.0f * 3.3 - 1 / 0.066f;
    fmt << current << " A";
    return fmt.str();
}

inline std::string fmt_core_power_on(uint16_t power_on) {
    if (power_on >= 900) {
        return "true";
    } else if (power_on <= 100) {
        return "false";
    } else {
        std::stringstream fmt;
        fmt << "undefined (raw: " << power_on << ")";
        return fmt.str();
    }
}

inline std::string fmt_imu_accel(uint16_t accel) {
    std::stringstream fmt;
    fmt << accel << " m/s^2";
    return fmt.str();
}

inline std::string fmt_imu_gyro(uint16_t gyro) {
    std::stringstream fmt;
    fmt << gyro << " deg/s";
    return fmt.str();
}

inline std::string fmt_mx_baud_rate(uint8_t baud_rate) {
    switch (baud_rate) {
        case 0: {
            return "9,600 Bd";
        }
        case 1: {
            return "57,600 Bd";
        }
        case 2: {
            return "115,200 Bd";
        }
        case 3: {
            return "1 MBd";
        }
        case 4: {
            return "2 MBd";
        }
        case 5: {
            return "3 MBd";
        }
        case 6: {
            return "4 MBd";
        }
        case 7: {
            return "4.5 MBd";
        }
        default: {
            std::stringstream fmt;
            fmt << "unknown (raw: " << baud_rate << ")";
            return fmt.str();
        }
    }
}

inline std::string fmt_mx_return_delay(uint8_t raw) {
    std::stringstream fmt;
    uint32_t return_delay = uint32_t(raw) * 2;
    fmt << return_delay << " us";
    return fmt.str();
}

inline std::string fmt_mx_drive_mode(uint8_t mode) {
    if (mode & 1) {
        return "reverse";
    } else {
        return "normal";
    }
}

inline std::string fmt_mx_operating_mode(uint8_t mode) {
    switch (mode) {
        case 0: {
            return "current control";
        }
        case 1: {
            return "velocity control";
        }
        case 3: {
            return "position control";
        }
        case 4: {
            return "extended position control (multi-turn)";
        }
        case 5: {
            return "current-based position control";
        }
        case 16: {
            return "PWM control (voltage control)";
        }
        default: {
            std::stringstream fmt;
            fmt << "unknown (raw: " << mode << ")";
            return fmt.str();
        }
    }
}

inline std::string fmt_mx_homing_offset(uint32_t offset) {
    std::stringstream fmt;
    float offset_deg = int32_t(offset) * 0.088f;
    fmt << offset_deg << " deg";
    return fmt.str();
}

inline std::string fmt_mx_moving_threshold(uint32_t threshold) {
    float threshold_rpm = threshold * 0.229f;
    std::stringstream fmt;
    fmt << threshold_rpm << " rpm";
    return fmt.str();
}

inline std::string fmt_mx_temp_limit(uint8_t limit) {
    std::stringstream fmt;
    fmt << std::to_string(limit) << " deg C";
    return fmt.str();
}

inline std::string fmt_mx_voltage_limit(uint16_t limit) {
    std::stringstream fmt;
    float limit_volts = limit * 0.1f;
    fmt << limit_volts << " V";
    return fmt.str();
}

inline std::string fmt_mx_pwm_limit(uint16_t limit) {
    std::stringstream fmt;
    float limit_percent = limit / 8.85f;
    fmt << limit_percent << "%";
    return fmt.str();
}

inline std::string fmt_mx_current_limit(uint16_t limit) {
    std::stringstream fmt;
    float limit_amps = limit * 0.00336f;
    fmt << limit_amps << " A";
    return fmt.str();
}

inline std::string fmt_mx_accel_limit(uint32_t limit) {
    std::stringstream fmt;
    float limit_rev = limit * 214.577f;
    fmt << limit_rev << " rev/min^2";
    return fmt.str();
}

inline std::string fmt_mx_velocity_limit(uint32_t limit) {
    float limit_rpm = limit * 0.229f;
    std::stringstream fmt;
    fmt << limit_rpm << " rpm";
    return fmt.str();
}

inline std::string fmt_mx_position_limit(uint32_t limit) {
    std::stringstream fmt;
    float limit_deg = limit * 0.088f;
    fmt << limit_deg << " deg";
    return fmt.str();
}

inline std::string fmt_mx_shutdown(uint8_t shutdown) {
    std::stringstream fmt;
    fmt << "0b" << std::bitset<8>(shutdown);
    return fmt.str();
}

inline std::string fmt_mx_status_return(uint8_t status_return) {
    switch (status_return) {
        case 0: {
            return "ping";
        }
        case 1: {
            return "ping/read";
        }
        case 2: {
            return "all";
        }
        default: {
            std::stringstream fmt;
            fmt << "unknown (raw: " << status_return << ")";
            return fmt.str();
        }
    }
}

inline std::string fmt_mx_hardware_error(uint8_t error) {
    std::stringstream fmt;
    fmt << "0b" << std::bitset<8>(error);
    return fmt.str();
}

inline std::string fmt_mx_velocity_i_gain(uint16_t raw) {
    return std::to_string(raw / 65536);
}

inline std::string fmt_mx_velocity_p_gain(uint16_t raw) {
    return std::to_string(raw / 128);
}

inline std::string fmt_mx_pos_d_gain(uint16_t raw) {
    return std::to_string(raw / 16);
}

inline std::string fmt_mx_pos_i_gain(uint16_t raw) {
    return std::to_string(raw / 65536);
}

inline std::string fmt_mx_pos_p_gain(uint16_t raw) {
    return std::to_string(raw / 128);
}

inline std::string fmt_mx_ff_2nd_gain(uint16_t raw) {
    return std::to_string(raw / 4);
}

inline std::string fmt_mx_ff_1st_gain(uint16_t raw) {
    return std::to_string(raw / 4);
}

inline std::string fmt_mx_watchdog(uint8_t raw) {
    std::stringstream fmt;
    int32_t value = raw;

    if (value > 0) {
        fmt << value * 20 << " ms";
        return fmt.str();
    } else if (value == 0) {
        return "disabled";
    } else if (value == -1) {
        return "watchdog error";
    } else {
        fmt << "unknown (raw: " << value << ")";
        return fmt.str();
    }
}

inline std::string fmt_mx_goal_pwm(uint16_t goal) {
    std::stringstream fmt;
    float goal_percent = goal / 8.85f;
    fmt << goal_percent << "%";
    return fmt.str();
}

inline std::string fmt_mx_goal_current(uint16_t goal) {
    std::stringstream fmt;
    float goal_amps = goal * 0.00336f;
    fmt << goal_amps << " A";
    return fmt.str();
}

inline std::string fmt_mx_goal_velocity(uint32_t goal) {
    std::stringstream fmt;
    float goal_rpm = goal * 0.229f;
    fmt << goal_rpm << " rpm";
    return fmt.str();
}

inline std::string fmt_mx_profile_velocity(uint32_t velocity) {
    std::stringstream fmt;
    float velocity_rpm = velocity * 0.229f;
    fmt << velocity_rpm << " rpm";
    return fmt.str();
}

inline std::string fmt_mx_goal_position(uint32_t goal) {
    std::stringstream fmt;
    float goal_deg = goal * 0.088f;
    fmt << goal_deg << " deg";
    return fmt.str();
}

inline std::string fmt_mx_tick(uint16_t tick) {
    std::stringstream fmt;
    fmt << tick << " ms";
    return fmt.str();
}

inline std::string fmt_mx_moving_status(uint8_t status) {
    std::stringstream fmt;
    fmt << "0b" << std::bitset<8>(status);
    return fmt.str();
}

inline std::string fmt_mx_present_pwm(uint16_t present) {
    std::stringstream fmt;
    float present_percent = present / 8.85f;
    fmt << present_percent << "%";
    return fmt.str();
}

inline std::string fmt_mx_present_current(uint16_t present) {
    std::stringstream fmt;
    float present_amps = present * 0.00336f;
    fmt << present_amps << " A";
    return fmt.str();
}

inline std::string fmt_mx_present_velocity(uint32_t present) {
    std::stringstream fmt;
    float present_rpm = present * 0.229f;
    fmt << present_rpm << " rpm";
    return fmt.str();
}

inline std::string fmt_mx_present_position(uint32_t present) {
    std::stringstream fmt;
    float present_deg = present * 0.088f;
    fmt << present_deg << " deg";
    return fmt.str();
}

inline std::string fmt_mx_present_voltage(uint16_t present) {
    std::stringstream fmt;
    float present_volts = present * 0.1f;
    fmt << present_volts << " V";
    return fmt.str();
}

inline std::string fmt_mx_present_temp(uint8_t present) {
    std::stringstream fmt;
    fmt << std::to_string(present) << " deg C";
    return fmt.str();
}

#endif
