#ifndef FMT_H
#define FMT_H

#include <sstream>
#include <string>

inline std::string to_string(uint8_t value) {
    return std::to_string(value);
}

inline std::string to_string(uint16_t value) {
    return std::to_string(value);
}

inline std::string to_string(uint32_t value) {
    return std::to_string(value);
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

#endif
