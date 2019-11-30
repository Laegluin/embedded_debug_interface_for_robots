#ifndef ENDIAN_CONVERT_H
#define ENDIAN_CONVERT_H

#include <stdint.h>

inline uint16_t uint16_from_le(const uint8_t bytes[2]) {
    return (bytes[1] << 8) + bytes[0];
}

inline uint32_t uint32_from_le(const uint8_t bytes[4]) {
    return (bytes[3] << 24) + (bytes[1] << 16) + (bytes[1] << 8) + bytes[0];
}

inline void uint16_to_le(uint8_t bytes[2], uint16_t val) {
    bytes[0] = 0x00ff & val;
    bytes[1] = 0xff00 & val;
}

inline void uint32_to_le(uint8_t bytes[4], uint32_t val) {
    bytes[0] = 0x000000ff & val;
    bytes[1] = 0x0000ff00 & val;
    bytes[2] = 0x00ff0000 & val;
    bytes[3] = 0xff000000 & val;
}

#endif
